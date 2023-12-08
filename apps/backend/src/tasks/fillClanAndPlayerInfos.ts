import { TaskRunStatus } from "@prisma/client";
import { Task } from "../task";
import { prisma } from "../prisma";

const STEP = 1000;

export const fillClanAndPlayerInfos: Task = async () => {
  for (let i = 0; i < 10; i++) {
    const clients = await prisma.gameServerClient.findMany({
      where: {
        OR: [
          {
            playerInfoGameType: null,
          },
          {
            clanInfoGameType: null,
          },
          {
            playerInfoMap: null,
          },
          {
            clanInfoMap: null,
          },
        ]
      },
      select: {
        id: true,
        playerName: true,
        clanName: true,
        snapshot: {
          select: {
            map: {
              select: {
                id: true,
                gameTypeName: true,
              },
            },
          },
        },
      },
      take: STEP,
    });

    if (clients.length === 0) {
      return TaskRunStatus.COMPLETED;
    }

    await prisma.$transaction([
      ...clients.map((client) => prisma.gameServerClient.update({
        where: {
          id: client.id,
        },
        data: {
          playerInfoGameType: {
            connectOrCreate: {
              where: {
                playerName_gameTypeName: {
                  playerName: client.playerName,
                  gameTypeName: client.snapshot.map.gameTypeName,
                },
              },
              create: {
                playerName: client.playerName,
                gameTypeName: client.snapshot.map.gameTypeName,
              }
            },
          },
          clanInfoGameType: client.clanName === null ? undefined : {
            connectOrCreate: {
              where: {
                clanName_gameTypeName: {
                  clanName: client.clanName,
                  gameTypeName: client.snapshot.map.gameTypeName,
                },
              },
              create: {
                clanName: client.clanName,
                gameTypeName: client.snapshot.map.gameTypeName,
              }
            },
          },
          playerInfoMap: {
            connectOrCreate: {
              where: {
                playerName_mapId: {
                  playerName: client.playerName,
                  mapId: client.snapshot.map.id,
                },
              },
              create: {
                playerName: client.playerName,
                mapId: client.snapshot.map.id,
              },
            },
          },
          clanInfoMap: client.clanName === null ? undefined : {
            connectOrCreate: {
              where: {
                clanName_mapId: {
                  clanName: client.clanName,
                  mapId: client.snapshot.map.id,
                },
              },
              create: {
                clanName: client.clanName,
                mapId: client.snapshot.map.id,
              },
            },
          },
        },
      })),
    ]);
  }

  return TaskRunStatus.INCOMPLETE;
}
