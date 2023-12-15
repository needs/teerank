import { TaskRunStatus } from "@prisma/client";
import { prisma } from "../prisma";
import { Task } from "../task";

export const addDefaultMasterServers: Task = async () => {
  await prisma.masterServer.createMany({
    data: [
      {
        address: 'master1.teeworlds.com',
        port: 8300,
      },
      {
        address: 'master2.teeworlds.com',
        port: 8300,
      },
      {
        address: 'master3.teeworlds.com',
        port: 8300,
      },
      {
        address: 'master4.teeworlds.com',
        port: 8300,
      },
    ],
    skipDuplicates: true,
  })

  return TaskRunStatus.COMPLETED;
}
