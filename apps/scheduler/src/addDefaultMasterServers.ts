import { prisma } from "./prisma";

// 0.6 masters listen on port 8300, 0.7 masters on port 8283
export async function addDefaultMasterServers() {
  await prisma.masterServer.createMany({
    data: [1, 2, 3, 4].flatMap((index) => [
      {
        address: `master${index}.teeworlds.com`,
        port: 8300,
      },
      {
        address: `master${index}.teeworlds.com`,
        port: 8283,
      },
    ]),
    skipDuplicates: true,
  })
}
