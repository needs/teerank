import { prisma } from "../prisma";
import { destroySockets, getReceivedPackets, sendData, setupSockets } from "../socket";
import { unpackGameServerInfoPackets } from "../packets/gameServerInfo";
import { wait } from "../utils";
import { differenceInMinutes } from "date-fns";
import { Task } from "../task";
import { TaskRunStatus } from "@prisma/client";

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

export const pollGameServers: Task = async () => {
  const sockets = setupSockets();

  const gameServers = await prisma.gameServer.findMany();

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

        await prisma.gameServerSnapshot.create({
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
              create: gameServerInfo.clients.map((client) => ({
                playerName: client.name,

                player: {
                  connectOrCreate: {
                    where: {
                      name: client.name,
                    },
                    create: {
                      name: client.name,
                    }
                  },
                },

                clan: client.clan === "" ? undefined : {
                  connectOrCreate: {
                    where: {
                      name: client.clan,
                    },
                    create: {
                      name: client.clan,
                    }
                  }
                },

                playerInfoGameType: {
                  connectOrCreate: {
                    where: {
                      playerName_gameTypeName: {
                        playerName: client.name,
                        gameTypeName: gameServerInfo.gameType,
                      },
                    },
                    create: {
                      playerName: client.name,
                      gameType: {
                        connect: {
                          name: gameServerInfo.gameType,
                        },
                      },
                    },
                  },
                },

                clanInfoGameType: client.clan === "" ? undefined : {
                  connectOrCreate: {
                    where: {
                      clanName_gameTypeName: {
                        clanName: client.clan,
                        gameTypeName: gameServerInfo.gameType,
                      },
                    },
                    create: {
                      clanName: client.clan,
                      gameType: {
                        connect: {
                          name: gameServerInfo.gameType,
                        },
                      },
                    },
                  },
                },

                playerInfoMap: {
                  connectOrCreate: {
                    where: {
                      playerName_mapId: {
                        playerName: client.name,
                        mapId: map.id,
                      }
                    },
                    create: {
                      playerName: client.name,
                      map: {
                        connect: {
                          id: map.id,
                        },
                      },
                    },
                  },
                },

                clanInfoMap: client.clan === "" ? undefined : {
                  connectOrCreate: {
                    where: {
                      clanName_mapId: {
                        clanName: client.clan,
                        mapId: map.id,
                      }
                    },
                    create: {
                      clanName: client.clan,
                      map: {
                        connect: {
                          id: map.id,
                        },
                      },
                    },
                  },
                },

                country: client.country,
                score: client.score,
                inGame: client.inGame,
              })),
            },
          },
        });

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
    } else {
      if (gameServer.offlineSince === null) {
        await prisma.gameServer.update({
          where: {
            id: gameServer.id,
          },
          data: {
            offlineSince: new Date(),
          },
        });
      }
    }
  }))

  destroySockets(sockets);

  return TaskRunStatus.INCOMPLETE;
}
