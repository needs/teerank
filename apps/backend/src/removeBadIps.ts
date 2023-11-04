import { isIP } from "net";
import { prisma } from "./prisma";

export async function removeBadIps() {
  const gameServers = await prisma.gameServer.findMany();

  for (const gameServer of gameServers) {
    const ip = gameServer.ip;
    if (isIP(ip) === 0) {
      console.log(`Removing game server ${ip}:${gameServer.port} because it has an invalid IP address`);
      await prisma.gameServer.delete({
        where: {
          id: gameServer.id,
        },
      });
    }
  }
}
