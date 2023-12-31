import { addMinutes } from "date-fns";
import { clearDatabase } from "../testSetup";
import { prisma } from "./prisma";

beforeEach(async () => {
  await clearDatabase();
});

test('race condition when poping jobs', async () => {
  const date = new Date();

  const masterServer = await prisma.masterServer.create({
    data: {
      address: 'localhost',
      port: 8300,
      pollingStartedAt: date,
    },
  });

  const updatedMasterServer = await prisma.masterServer.update({
    where: {
      id: masterServer.id,
      pollingStartedAt: null,
    },
    data: {
      pollingStartedAt: addMinutes(date, 1)
    },
  }).catch(() => null);

  expect(updatedMasterServer).toBeNull();

  const updatedMasterServer2 = await prisma.masterServer.findUniqueOrThrow({
    where: {
      id: masterServer.id,
    },
  });

  expect(updatedMasterServer2.pollingStartedAt).toStrictEqual(date);
});
