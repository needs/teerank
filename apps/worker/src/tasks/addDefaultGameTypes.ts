import { RankMethod, TaskRunStatus } from "@prisma/client";
import { prisma } from "../prisma";
import { Task } from "../task";

export const addDefaultGameTypes: Task = async () => {
  const gameTypesElo = [
    "DM", "CTF", "TDM", "DM*", "DM+", "DM++", "fng2", "fng2+", "iCTF*", "iCTF+", "iCTFX", "iDM*", "iDM+", "zCatch", "zCatch/TeeVi"
  ];

  const gameTypesTime = [
    "DDraceNetwork", "Gores", "TestDDraceNetwor", "TestGores", "Race", "iDDRace", "XXLDDRace", "PPRace", "TestDDNet++"
  ];

  await prisma.$transaction([
    ...gameTypesElo.map(gameType => prisma.gameType.upsert({
      where: {
        name: gameType,
      },
      update: {
        rankMethod: RankMethod.ELO,
      },
      create: {
        name: gameType,
        rankMethod: RankMethod.ELO,
      },
    })),

    ...gameTypesTime.map(gameType => prisma.gameType.upsert({
      where: {
        name: gameType,
      },
      update: {
        rankMethod: RankMethod.TIME,
      },
      create: {
        name: gameType,
        rankMethod: RankMethod.TIME,
      },
    })),
  ]);

  return TaskRunStatus.COMPLETED;
}
