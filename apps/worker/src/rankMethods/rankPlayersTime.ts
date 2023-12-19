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

  const playerInfoMaps = await prisma.$transaction(
    snapshot.clients.map((client) => prisma.playerInfoMap.upsert({
      where: {
        playerName_mapId: {
          playerName: client.playerName,
          mapId: snapshot.mapId,
        },
      },
      select: {
        id: true,
        playerName: true,
        rating: true,
      },
      update: {},
      create: {
        playerName: client.playerName,
        mapId: snapshot.mapId,
      },
    })),
  );

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

  await prisma.$transaction(Array.from(newPlayerTimes.entries()).map(([playerName, newTime]) =>
    prisma.playerInfoMap.update({
      where: {
        playerName_mapId: {
          playerName,
          mapId: snapshot.mapId,
        },
      },
      data: {
        rating: newTime,
      },
    })
  ));
}
