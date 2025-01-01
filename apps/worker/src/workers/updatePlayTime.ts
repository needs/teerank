import { prisma } from "../prisma";
import { differenceInSeconds, minutesToSeconds } from "date-fns";
import { removeDuplicatedClients } from "../utils";
import { Worker } from "bullmq";
import { bullmqConnection, QUEUE_NAME_UPDATE_PLAY_TIME } from "@teerank/teerank";
import { getEnvInt } from "@teerank/teerank";

const UPDATE_PLAY_TIME_CONCURRENCY = getEnvInt('UPDATE_PLAY_TIME_CONCURRENCY', 20);

export async function updatePlayTime(snapshotId: number) {
  const snapshot = await prisma.gameServerSnapshot.findUniqueOrThrow({
    where: {
      id: snapshotId,
    },
    select: {
      id: true,
      createdAt: true,
      gameServerId: true,
      mapId: true,
      map: {
        select: {
          gameTypeName: true,
        },
      },
      clients: {
        select: {
          playerName: true,
          clanName: true,
        },
      },
    },
  });

  const snapshotBefore = await prisma.gameServerSnapshot.findFirst({
    select: {
      createdAt: true,
    },
    where: {
      createdAt: {
        lt: snapshot.createdAt,
      },
      gameServerId: snapshot.gameServerId,
    },
    orderBy: {
      createdAt: 'desc',
    },
  });

  const deltaSecond = snapshotBefore === null ? 0 : differenceInSeconds(snapshot.createdAt, snapshotBefore.createdAt);
  const deltaPlayTime = deltaSecond > 10 * 60 ? 5 * 60 : deltaSecond;
  const clients = removeDuplicatedClients(snapshot.clients);

  for (const client of clients) {
    await Promise.all([
      prisma.playerInfoMap.upsert({
        where: {
          playerName_mapId: {
            mapId: snapshot.mapId,
            playerName: client.playerName,
          },
        },
        update: {
          playTime: {
            increment: deltaPlayTime,
          },
        },
        create: {
          player: {
            connect: {
              name: client.playerName
            }
          },
          map: {
            connect: {
              id: snapshot.mapId,
            },
          },
          playTime: deltaPlayTime,
        },
      }),

      prisma.playerInfoGameType.upsert({
        where: {
          playerName_gameTypeName: {
            gameTypeName: snapshot.map.gameTypeName,
            playerName: client.playerName,
          },
        },
        update: {
          playTime: {
            increment: deltaPlayTime,
          },
        },
        create: {
          player: {
            connect: {
              name: client.playerName
            }
          },
          gameType: {
            connect: {
              name: snapshot.map.gameTypeName,
            },
          },
          playTime: deltaPlayTime,
        },
      }),

      prisma.player.update({
        where: {
          name: client.playerName,
        },
        data: {
          playTime: {
            increment: deltaPlayTime,
          },
        },
      }),

      ...(client.clanName === null ? [] : [
        prisma.clanInfoMap.upsert({
          where: {
            clanName_mapId: {
              mapId: snapshot.mapId,
              clanName: client.clanName,
            },
          },
          update: {
            playTime: {
              increment: deltaPlayTime,
            },
          },
          create: {
            clan: {
              connect: {
                name: client.clanName
              }
            },
            map: {
              connect: {
                id: snapshot.mapId,
              },
            },
            playTime: deltaPlayTime,
          },
        }),

        prisma.clanInfoGameType.upsert({
          where: {
            clanName_gameTypeName: {
              gameTypeName: snapshot.map.gameTypeName,
              clanName: client.clanName,
            },
          },
          update: {
            playTime: {
              increment: deltaPlayTime,
            },
          },
          create: {
            clan: {
              connect: {
                name: client.clanName
              }
            },
            gameType: {
              connect: {
                name: snapshot.map.gameTypeName,
              },
            },
            playTime: deltaPlayTime,
          },
        }),

        prisma.clan.update({
          where: {
            name: client.clanName,
          },
          data: {
            playTime: {
              increment: deltaPlayTime,
            },
          },
        }),

        prisma.clanPlayerInfo.upsert({
          where: {
            clanName_playerName: {
              clanName: client.clanName,
              playerName: client.playerName,
            },
          },
          update: {
            playTime: {
              increment: deltaPlayTime,
            },
          },
          create: {
            clan: {
              connect: {
                name: client.clanName
              }
            },
            player: {
              connect: {
                name: client.playerName
              }
            },
            playTime: deltaPlayTime,
          },
        }),
      ])
    ])
  }

  await Promise.all([
    prisma.gameType.update({
      where: {
        name: snapshot.map.gameTypeName,
      },
      data: {
        playTime: {
          increment: deltaPlayTime * clients.length,
        },
      },
    }),

    prisma.map.update({
      where: {
        id: snapshot.mapId,
      },
      data: {
        playTime: {
          increment: deltaPlayTime * clients.length,
        },
      },
    }),

    prisma.gameServer.update({
      where: {
        id: snapshot.gameServerId,
      },
      data: {
        playTime: {
          increment: deltaPlayTime * clients.length,
        },
      },
    }),
  ]);
}

export async function startUpdatePlayTimeWorker() {
  return new Worker(QUEUE_NAME_UPDATE_PLAY_TIME, (job) => updatePlayTime(job.data.snapshotId), {
    connection: bullmqConnection,
    concurrency: UPDATE_PLAY_TIME_CONCURRENCY,
    removeOnComplete: {
      age: minutesToSeconds(10),
    },
  });
}
