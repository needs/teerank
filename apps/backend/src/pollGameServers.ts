import { prisma } from "./prisma";
import { destroySockets, getReceivedPackets, sendData, setupSockets } from "./socket";
import { unpackGameServerInfoPackets } from "./packets/gameServerInfo";
import { wait } from "./utils";

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
  const sockets = setupSockets();

  const gameServers = await prisma.gameServer.findMany();

  await Promise.all(gameServers.map(async (gameServer, index) => {
    await wait(index * 100);

    sendData(sockets, PACKET_GETINFO, gameServer.ip, gameServer.port);
    sendData(sockets, PACKET_GETINFO64, gameServer.ip, gameServer.port);

    await wait(2000);

    const receivedPackets = getReceivedPackets(sockets, gameServer.ip, gameServer.port);

    if (receivedPackets !== undefined) {
      try {
        const gameServerInfo = unpackGameServerInfoPackets(receivedPackets.packets)

        await prisma.gameServerSnapshot.create({
          data: {
            gameServer: {
              connect: {
                id: gameServer.id,
              },
            },

            version: gameServerInfo.version,
            name: gameServerInfo.name,

            map: {
              connectOrCreate: {
                where: {
                  name_gameTypeName: {
                    name: gameServerInfo.map,
                    gameTypeName: gameServerInfo.gameType,
                  },
                },
                create: {
                  name: gameServerInfo.map,
                  gameType: {
                    connectOrCreate: {
                      where: {
                        name: gameServerInfo.gameType,
                      },
                      create: {
                        name: gameServerInfo.gameType,
                      },
                    },
                  },
                },
              },
            },
            numPlayers: gameServerInfo.numPlayers,
            maxPlayers: gameServerInfo.maxPlayers,
            numClients: gameServerInfo.numClients,
            maxClients: gameServerInfo.maxClients,

            clients: {
              create: gameServerInfo.clients.map((client) => ({
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
                clan: client.clan,
                country: client.country,
                score: client.score,
                inGame: client.inGame,
              })),
            },
          },
        });

        console.log(`${gameServer.ip}:${gameServer.port}: Polled (${gameServerInfo.gameType} ${gameServerInfo.map} ${gameServerInfo.numClients}/${gameServerInfo.maxClients})`)
      } catch (e) {
        console.warn(`${gameServer.ip}:${gameServer.port}: ${e.message}`)
      }
    } else {
      console.log(`${gameServer.ip}:${gameServer.port}: No response`);
    }
  }))

  destroySockets(sockets);
}
