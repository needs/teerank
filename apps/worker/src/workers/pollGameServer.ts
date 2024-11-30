import { prisma } from "../prisma";
import { resetPackets, getReceivedPackets, sendData, setupSockets, listenForPackets } from "../socket";
import { unpackGameServerInfoPackets } from "../packets/gameServerInfo";
import { differenceInMinutes } from "date-fns";
import { QUEUE_NAME_POLL_GAME_SERVER, wait } from "@teerank/teerank";
import { GameServer, GameServerState } from "@prisma/client";
import { Job, Worker } from "bullmq";
import { bullmqConnection } from "@teerank/teerank";

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

function skipPolling(gameServer: GameServer & { gameServerState: GameServerState | null }) {
  if (gameServer.gameServerState === null) {
    const now = new Date();

    const maxMinutes = 24 * 60;
    const lastSeenAtMinutes = Math.min(maxMinutes, differenceInMinutes(now, gameServer.lastSeenAt));

    // The longer offline, the less odds to poll, range from 0.95 to 0.05
    const odds = 0.05 + 0.9 * (lastSeenAtMinutes / maxMinutes);

    return Math.random() >= odds;
  } else {
    return false;
  }
}

async function processor(job: Job) {
  const gameServer =
    await prisma.gameServer.findUniqueOrThrow({
      where: {
        ip_port: {
          ip: job.data.ip,
          port: job.data.port,
        }
      },
      include: {
        gameServerState: true,
      },
    });


  if (skipPolling(gameServer)) {
    return;
  }

  const sockets = await setupSockets;

  listenForPackets(sockets, gameServer.ip, gameServer.port);

  sendData(sockets, PACKET_GETINFO, gameServer.ip, gameServer.port);
  sendData(sockets, PACKET_GETINFO64, gameServer.ip, gameServer.port);

  await wait(2000);

  const receivedPackets = getReceivedPackets(sockets, gameServer.ip, gameServer.port);

  if (receivedPackets.packets.length > 0) {
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

      await Promise.all(gameServerInfo.clients.map((client) => prisma.playerInfoGameType.upsert({
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

      await Promise.all(uniqClans.map((clan) => prisma.clanInfoGameType.upsert({
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

      await Promise.all(gameServerInfo.clients.map((client) => prisma.playerInfoMap.upsert({
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

      await Promise.all(uniqClans.map((clan) => prisma.clanInfoMap.upsert({
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

      await prisma.$transaction(async (tx) => {
        // I couldn't find an elegant way to do replace all clients for an
        // existing game server state so we delete the existing one and create
        // a new one.  Do it in a transaction to avoid race conditions.

        if (gameServer.gameServerState !== null) {
          await tx.gameServerState.delete({
            where: {
              id: gameServer.gameServerState.id,
            },
          });
        }

        await tx.gameServerState.create({
          data: {
            gameServer: {
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
      });

      await Promise.all(
        snapshot.clients.map((client) =>
          prisma.player.update({
            where: {
              name: client.playerName,
            },
            data: {
              lastSeenAt: new Date(),
            },
          })
        )
      );

      await prisma.gameServer.update({
        where: {
          id: gameServer.id,
        },
        data: {
          lastSeenAt: new Date(),
        },
      });
    } catch (e) {
      console.warn(`${gameServer.ip}:${gameServer.port}: ${e}`)
    }
  } else if (gameServer.gameServerState !== null) {
    await prisma.gameServerState.delete({
      where: {
        id: gameServer.gameServerState.id,
      },
    });
  }

  resetPackets(sockets, gameServer.ip, gameServer.port);
}

export async function startPollGameServerWorker() {
  new Worker(QUEUE_NAME_POLL_GAME_SERVER, processor, {
    connection: bullmqConnection,
    concurrency: 20,
  });
}
