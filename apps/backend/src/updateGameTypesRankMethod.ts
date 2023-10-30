import { prisma } from "./prisma";

export async function updateGameTypesRankMethod() {
  await prisma.gameType.createMany({
    data: [
      {
        name: "DM",
        rankMethod: "ELO",
      },
      {
        name: "CTF",
        rankMethod: "ELO",
      },
    ],
    skipDuplicates: true,
  });
}
