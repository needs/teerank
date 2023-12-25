import { prisma } from "../prisma";
import { destroySockets, getReceivedPackets, sendData, setupSockets } from "../socket";
import { unpackGameServerInfoPackets } from "../packets/gameServerInfo";
import { wait } from "../utils";
import { differenceInMinutes, subMinutes } from "date-fns";
import { compact } from "lodash";

function stringToCharCode(str: string) {
  return str.split('').map((char) => char.charCodeAt(0));
}

const PACKET_HEADER = Buffer.from([
  ...stringToCharCode('xe'),
  0xff,
  0xff,
  0xff,
  0xff,
]);

const PACKET_GETINFO = Buffer.from([
  ...PACKET_HEADER,
  0xff,
  0xff,
  0xff,
  0xff,
  ...stringToCharCode('gie3'),
  0
]);

const PACKET_GETINFO64 = Buffer.from([
  ...PACKET_HEADER,
  0xff,
  0xff,
  0xff,
  0xff,
  ...stringToCharCode('fstd'),
  0
]);

export async function pollGameServers() {
  const gameServerCandidates = await prisma.gameServer.findMany({
    select: {
      id: true,
    },
    where: {
      OR: [{
        polledAt: {
          lt: subMinutes(new Date(), 5)
        }
      }, {
        polledAt: null,
      }],
      pollingStartedAt: null,
    },
    orderBy: {
      createdAt: 'desc',
    },
    take: 100,
  });

  if (gameServerCandidates.length === 0) {
    return false;
  }

  const gameServers = compact(await Promise.all(gameServerCandidates.map(async ({ id }) => {
    return await prisma.gameServer.update({
      where: {
        id,
        pollingStartedAt: null,
      },
      data: {
        pollingStartedAt: new Date(),
      },
    }).catch(() => null);
  })));

  if (gameServers.length === 0) {
    return false;
  }

  const sockets = setupSockets();

  await Promise.all(gameServers.filter((gameServer) => {
    if (gameServer.offlineSince !== null) {
      const offlineSince = new Date(gameServer.offlineSince);
      const now = new Date();

      const maxMinutes = 24 * 60;
      const offlineSinceMinutes = Math.min(maxMinutes, differenceInMinutes(now, offlineSince));

      // The longer offline, the less odds to poll, range from 0.95 to 0.05
      const odds = 0.05 + 0.9 * (offlineSinceMinutes / maxMinutes);

      return Math.random() < odds;
    } else {
      return true;
    }
  }).map(async (gameServer, index) => {
    await wait(index * 100);

    sendData(sockets, PACKET_GETINFO, gameServer.ip, gameServer.port);
    sendData(sockets, PACKET_GETINFO64, gameServer.ip, gameServer.port);

    await wait(2000);

    const receivedPackets = getReceivedPackets(sockets, gameServer.ip, gameServer.port);

    if (receivedPackets !== undefined) {
      try {
        const gameServerInfo = unpackGameServerInfoPackets(receivedPackets.packets)

        const map = await prisma.map.upsert({
          select: {
            id: true,
          },
          where: {
            name_gameTypeName: {
              name: gameServerInfo.map,
              gameTypeName: gameServerInfo.gameType,
            },
          },
          update: {},
          create: {
            name: gameServerInfo.map,
            gameType: {
              connectOrCreate: {
                create: {
                  name: gameServerInfo.gameType,
                },
                where: {
                  name: gameServerInfo.gameType,
                },
              },
            },
          },
        });

        await prisma.player.createMany({
          data: gameServerInfo.clients.map((client) => ({
            name: client.name,
          })),
          skipDuplicates: true,
        });

        const uniqClans = [...new Set(gameServerInfo.clients.map((client) => client.clan))];

        await prisma.clan.createMany({
          data: uniqClans.map((clan) => ({
            name: clan,
          })),
          skipDuplicates: true,
        });

        await prisma.$transaction(gameServerInfo.clients.map((client) => prisma.playerInfoGameType.upsert({
          select: {
            id: true,
          },
          where: {
            playerName_gameTypeName: {
              playerName: client.name,
              gameTypeName: gameServerInfo.gameType,
            },
          },
          update: {},
          create: {
            playerName: client.name,
            gameTypeName: gameServerInfo.gameType,
          },
        })));

        await prisma.$transaction(uniqClans.map((clan) => prisma.clanInfoGameType.upsert({
          select: {
            id: true,
          },
          where: {
            clanName_gameTypeName: {
              clanName: clan,
              gameTypeName: gameServerInfo.gameType,
            },
          },
          update: {},
          create: {
            clanName: clan,
            gameTypeName: gameServerInfo.gameType,
          },
        })));

        await prisma.$transaction(gameServerInfo.clients.map((client) => prisma.playerInfoMap.upsert({
          select: {
            id: true,
          },
          where: {
            playerName_mapId: {
              playerName: client.name,
              mapId: map.id,
            },
          },
          update: {},
          create: {
            playerName: client.name,
            mapId: map.id,
          },
        })));

        await prisma.$transaction(uniqClans.map((clan) => prisma.clanInfoMap.upsert({
          select: {
            id: true,
          },
          where: {
            clanName_mapId: {
              clanName: clan,
              mapId: map.id,
            },
          },
          update: {},
          create: {
            clanName: clan,
            mapId: map.id,
          },
        })));

        const snapshot = await prisma.gameServerSnapshot.create({
          select: {
            clients: {
              select: {
                playerName: true,
                id: true
              }
            }
          },
          data: {
            gameServer: {
              connect: {
                id: gameServer.id,
              },
            },

            gameServerLast: {
              connect: {
                id: gameServer.id,
              },
            },

            version: gameServerInfo.version,
            name: gameServerInfo.name,

            map: {
              connect: {
                id: map.id,
              },
            },
            numPlayers: gameServerInfo.numPlayers,
            maxPlayers: gameServerInfo.maxPlayers,
            numClients: gameServerInfo.numClients,
            maxClients: gameServerInfo.maxClients,

            clients: {
              createMany: {
                data: gameServerInfo.clients.map((client) => ({
                  playerName: client.name,
                  clanName: client.clan === "" ? undefined : client.clan,
                  country: client.country,
                  score: client.score,
                  inGame: client.inGame,
                })),
              },
            }
          },
        });

        await prisma.$transaction(
          snapshot.clients.map((client) =>
            prisma.player.update({
              where: {
                name: client.playerName,
              },
              data: {
                lastGameServerClient: {
                  connect: {
                    id: client.id,
                  }
                }
              },
            })
          )
        );

        await prisma.gameServer.update({
          where: {
            id: gameServer.id,
          },
          data: {
            offlineSince: null,
          },
        });
      } catch (e) {
        console.warn(`${gameServer.ip}:${gameServer.port}: ${e}`)
      }
    } else if (gameServer.offlineSince === null) {
      await prisma.gameServer.update({
        where: {
          id: gameServer.id,
        },
        data: {
          offlineSince: new Date(),
        },
      });
    }

    await prisma.gameServer.update({
      where: {
        id: gameServer.id,
      },
      data: {
        polledAt: new Date(),
        pollingStartedAt: null,
      },
    });
  }))

  destroySockets(sockets);

  return true;
}
