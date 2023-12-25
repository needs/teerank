import { prisma } from "../prisma";
import { differenceInSeconds } from "date-fns";
import { removeDuplicatedClients } from "../utils";
import { compact } from "lodash";

export async function updatePlayTimes() {
  const snapshotCandidates = await prisma.gameServerSnapshot.findMany({
    where: {
      playTimingStartedAt: null,
    },
    select: {
      id: true,
    },
    orderBy: {
      createdAt: 'desc',
    },
    take: 100,
  });

  if (snapshotCandidates.length === 0) {
    return false;
  }

  const snapshots = compact(await Promise.all(snapshotCandidates.map(async ({ id }) => {
    return await prisma.gameServerSnapshot.update({
      where: {
        id,
        playTimingStartedAt: null,
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
            player: {
              select: {
                clanSnapshotCreatedAt: true,
              }
            },
            clanName: true,
          },
        },
      },
      data: {
        playTimingStartedAt: new Date(),
      },
    }).catch(() => null);
  })));

  if (snapshots.length === 0) {
    return false;
  }

  for (const snapshot of snapshots) {
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
      await prisma.$transaction([
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

        prisma.player.updateMany({
          where: {
            name: client.playerName,
            clanSnapshotCreatedAt: {
              lt: snapshot.createdAt,
            }
          },
          data: {
            clanSnapshotCreatedAt: snapshot.createdAt,
            clanName: client.clanName
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

    await prisma.$transaction([
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
    ]);

    await prisma.gameServerSnapshot.update({
      where: {
        id: snapshot.id,
      },
      data: {
        playTimedAt: new Date(),
      },
    });
  }

  return true;
}
