import { prismaMock } from "../../test/mockPrisma";
import { fillClanActivePlayerCount } from "./fillClanActivePlayerCount";

describe('fillClanActivePlayerCount', () => {
  test('one additional player in existing clan', async () => {
    prismaMock.clan.findMany.mockResolvedValue(
      [
        {
          name: 'clan1',
          id: 1,
          createdAt: new Date(),
          updatedAt: new Date(),
          playTime: BigInt(0),
          activePlayerCount: 100,
        }
      ]
    );

    prismaMock.player.groupBy.mockResolvedValue(
      [
        {
          name: 'player1',
          id: 1,
          createdAt: new Date(),
          updatedAt: new Date(),
          playTime: BigInt(0),
          clanName: 'clan1',
          lastSeenAt: new Date(),

          _count: { id: 101 },
          _avg: { id: 101 },
          _max: { id: 101 },
          _min: { id: 101 },
          _sum: { id: 101 },
        }
      ]
    );

    await fillClanActivePlayerCount();

    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: 'clan1' },
      data: { activePlayerCount: 101 },
    });
  });

  test('one player in a new clan', async () => {
    prismaMock.clan.findMany.mockResolvedValue([]);

    prismaMock.player.groupBy.mockResolvedValue(
      [
        {
          name: 'player1',
          id: 1,
          createdAt: new Date(),
          updatedAt: new Date(),
          playTime: BigInt(0),
          clanName: 'clan1',
          lastSeenAt: new Date(),

          _count: { id: 1 },
          _avg: { id: 1 },
          _max: { id: 1 },
          _min: { id: 1 },
          _sum: { id: 1 },
        }
      ]
    );

    await fillClanActivePlayerCount();

    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: 'clan1' },
      data: { activePlayerCount: 1 },
    });
  });

  test('player changed clan', async () => {
    prismaMock.clan.findMany.mockResolvedValue(
      [
        {
          name: 'clan1',
          id: 1,
          createdAt: new Date(),
          updatedAt: new Date(),
          playTime: BigInt(0),
          activePlayerCount: 1,
        }
      ]
    );

    prismaMock.player.groupBy.mockResolvedValue(
      [
        {
          name: 'player1',
          id: 1,
          createdAt: new Date(),
          updatedAt: new Date(),
          playTime: BigInt(0),
          clanName: 'clan2',
          lastSeenAt: new Date(),

          _count: { id: 1 },
          _avg: { id: 1 },
          _max: { id: 1 },
          _min: { id: 1 },
          _sum: { id: 1 },
        }
      ]
    );

    await fillClanActivePlayerCount();

    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: 'clan2' },
      data: { activePlayerCount: 1 },
    });

    expect(prismaMock.clan.update).toHaveBeenCalledWith({
      where: { name: 'clan1' },
      data: { activePlayerCount: 0 },
    });
  });
});
