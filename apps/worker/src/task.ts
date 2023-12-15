import { TaskRunStatus } from '@prisma/client';
import { prisma } from './prisma';

export type Task = () => Promise<TaskRunStatus> | TaskRunStatus;
export type Tasks = Record<string, Task>;

export async function updateTasks(tasks: Tasks) {
  await prisma.task.deleteMany({
    where: {
      name: {
        notIn: Object.keys(tasks),
      },
    },
  });

  await prisma.$transaction(
    Object.values(tasks).map((taskName, index) =>
      prisma.task.upsert({
        where: {
          name: taskName.name,
        },
        update: {
          name: taskName.name,
          index,
        },
        create: {
          name: taskName.name,
          index,
        },
      })
    )
  );

  await prisma.taskRun.updateMany({
    where: {
      status: null,
      finishedAt: null,
    },
    data: {
      status: TaskRunStatus.FAILED,
      finishedAt: new Date(),
    },
  });
}

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

    console.log(`Started ${taskName}`);
    console.time(`Finished ${taskName}`);

    let status: TaskRunStatus = TaskRunStatus.FAILED;

    try {
      status = await task();
    } catch (e) {
      console.error(e);
    }

    console.timeEnd(`Finished ${taskName}`);

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
      console.log(`Task ${taskName} finished and will not run again`);
    } else {
      console.log(`Task ${taskName} is scheduled to run again`);
    }
  }
}
