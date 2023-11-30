import { TaskRunStatus } from '@prisma/client';
import { prisma } from './prisma';

export type Task = () => Promise<TaskRunStatus> | TaskRunStatus;
export type Tasks = Record<string, Task>;

export async function runTasks(tasks: Tasks) {
  const completedTasks = await prisma.task.findMany({
    select: {
      name: true,
    },
    where: {
      completedAt: {
        not: null,
      },
    },
  });

  const completedTaskNames = new Set(completedTasks.map(completedTask => completedTask.name));
  const incompleteTasks = Object.entries(tasks).filter(([scriptName]) => {
    return completedTaskNames.has(scriptName) === false;
  });

  for (const [taskName, task] of incompleteTasks) {
    const taskRun = await prisma.taskRun.create({
      data: {
        task: {
          connectOrCreate: {
            where: {
              name: taskName,
            },
            create: {
              name: taskName,
            },
          },
        },
      },
    });

    console.time(`Running ${taskName}`);
    const status = await task();
    console.timeEnd(`Running ${taskName}`);

    await prisma.taskRun.update({
      where: {
        id: taskRun.id,
      },
      data: {
        finishedAt: new Date(),
        status,
      },
    });

    if (status === TaskRunStatus.COMPLETED) {
      await prisma.task.update({
        where: {
          name: taskName,
        },
        data: {
          completedAt: new Date(),
        },
      });
      console.log(`Script ${taskName} finished and will not run again`);
    } else {
      console.log(`Script ${taskName} is scheduled to run again`);
    }
  }
}
