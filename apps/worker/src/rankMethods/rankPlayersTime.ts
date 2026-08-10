import { setPlayerInfoMapRatings } from "@prisma/client/sql";
import { sortBy } from "lodash";
import { prisma } from "../prisma";

export const rankPlayersTime = async (snapshotId: number) => {
  const snapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshotId,
    },
    select: {
      clients: {
        select: {
          playerName: true,
          score: true,
          inGame: true,
        }
      },
      mapId: true,
    },
  });

  if (snapshot.clients.length === 0) {
    return;
  }

  const playerNames = [...new Set(snapshot.clients.map((client) => client.playerName))].sort();

  const where = {
    mapId: snapshot.mapId,
    playerName: {
      in: playerNames,
    },
  };
  const select = {
    playerName: true,
    rating: true,
  };

  // Fetch the info rows, creating the (rarely) missing ones first — a plain
  // read in steady state instead of one upsert per player.
  let playerInfoMaps = await prisma.playerInfoMap.findMany({ where, select });

  if (playerInfoMaps.length < playerNames.length) {
    const existing = new Set(playerInfoMaps.map(({ playerName }) => playerName));
    const missing = playerNames.filter((playerName) => !existing.has(playerName));

    await prisma.playerInfoMap.createMany({
      data: missing.map((playerName) => ({ playerName, mapId: snapshot.mapId })),
      skipDuplicates: true,
    });
    playerInfoMaps = await prisma.playerInfoMap.findMany({ where, select });
  }

  const playerTimes = new Map(playerInfoMaps.map(({ playerName, rating }) => [playerName, rating]));
  const newPlayerTimes = new Map<string, number>();

  for (const client of snapshot.clients) {
    if (!client.inGame || Math.abs(client.score) === 9999 || client.playerName === '(connecting)') {
      continue;
    }

    const newTime = -Math.abs(client.score);
    const currentTime = playerTimes.get(client.playerName) ?? undefined;

    if (currentTime === undefined || newTime > currentTime) {
      newPlayerTimes.set(client.playerName, newTime);
    }
  }

  if (newPlayerTimes.size === 0) {
    return;
  }

  // One multi-row update, sorted by the conflict key to avoid deadlocks
  // between concurrent workers.
  const updates = sortBy([...newPlayerTimes.entries()], ([playerName]) => playerName);

  await prisma.$queryRawTyped(setPlayerInfoMapRatings(
    updates.map(([playerName]) => playerName),
    snapshot.mapId,
    updates.map(([, rating]) => rating),
  ));
}
