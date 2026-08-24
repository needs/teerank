import { STUB_MIN_POLL_COUNT, STUB_OCCURRENCE_RATIO } from '@teerank/teerank';
import {
  countPlayerPartnersWithoutShared,
  listPlayerPartnersWithoutShared,
} from '@prisma/client/sql';
import prisma from './prisma';

export type Teammate = {
  name: string;
  clan: string | null;
  lastSeenAt: Date;
  playTime: number;
  lastPlayedAt: Date;
  gameServers: { ip: string; port: number }[];
};

export async function countTeammates(playerId: number) {
  const counts = await prisma.$queryRawTyped(
    countPlayerPartnersWithoutShared(
      playerId,
      STUB_MIN_POLL_COUNT,
      STUB_OCCURRENCE_RATIO
    )
  );

  return Number(counts[0]?.count ?? 0);
}

export async function getTeammates(
  playerId: number,
  { skip, take }: { skip: number; take: number }
): Promise<Teammate[]> {
  const partnerRows = await prisma.$queryRawTyped(
    listPlayerPartnersWithoutShared(
      playerId,
      STUB_MIN_POLL_COUNT,
      STUB_OCCURRENCE_RATIO,
      take,
      skip
    )
  );

  const partners = await prisma.player.findMany({
    where: {
      id: { in: partnerRows.map((row) => row.partnerId) },
    },
    select: {
      id: true,
      name: true,
      clanName: true,
      lastSeenAt: true,
      gameServerStateClients: {
        select: {
          gameServerState: {
            select: {
              gameServer: {
                select: { ip: true, port: true },
              },
            },
          },
        },
      },
    },
  });

  const partnersById = new Map(partners.map((partner) => [partner.id, partner]));

  return partnerRows.flatMap((row) => {
    const partner = partnersById.get(row.partnerId);

    if (partner === undefined) {
      return [];
    }

    return {
      name: partner.name,
      clan: partner.clanName,
      lastSeenAt: partner.lastSeenAt,
      playTime: row.playTime,
      lastPlayedAt: row.lastPlayedAt,
      gameServers: partner.gameServerStateClients.map((client) => ({
        ip: client.gameServerState.gameServer.ip,
        port: client.gameServerState.gameServer.port,
      })),
    };
  });
}
