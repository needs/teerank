import { prisma } from "./prisma";

export async function updateGameTypesRankMethod() {
  const gameTypes = [
    "DM", "CTF", "TDM", "DM*", "DM+", "DM++", "fng2", "fng2+", "iCTF*", "iCTF+", "iCTFX", "iDM*", "iDM+", "zCatch", "zCatch/TeeVi"
  ];

  await prisma.gameType.createMany({
    data: gameTypes.map(gameType => ({
      name: gameType,
      rankMethod: "ELO",
    })),
    skipDuplicates: true,
  });
}
