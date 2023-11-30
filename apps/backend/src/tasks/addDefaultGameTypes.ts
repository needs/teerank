import { TaskRunStatus } from "@prisma/client";
import { prisma } from "../prisma";
import { Task } from "../task";

export const addDefaultGameTypes: Task = async () => {
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

  return TaskRunStatus.COMPLETED;
}
