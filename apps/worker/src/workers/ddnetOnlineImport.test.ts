import { prismaMock } from "../../test/mockPrisma";
import { ddnetOnlineImport } from "./ddnetOnlineImport";

function csvResponse(text: string) {
  return new Response(text, { status: 200 });
}

describe('ddnetOnlineImport', () => {
  beforeEach(() => {
    prismaMock.ddnetState.findUnique.mockResolvedValue(null);
    prismaMock.$transaction.mockImplementation(((callback: (tx: typeof prismaMock) => Promise<unknown>) =>
      callback(prismaMock)) as never);
  });

  it('merges a sample logged after the next day began into its own day', async () => {
    const bycountry = [
      '2021-10-21 23:56,GER:10,USA:2',
      '2021-10-22 00:00,GER:4,USA:0',
      '2021-10-21 23:58,GER:20,USA:2',
      '2021-10-22 00:02,GER:6,USA:0',
      '2021-10-23 00:00,GER:1,USA:1',
    ].join('\n') + '\n';

    global.fetch = jest.fn(async (url: string) =>
      csvResponse(url.endsWith('/bycountry') ? bycountry : '')
    ) as never;

    await ddnetOnlineImport();

    const rows = prismaMock.ddnetOnlineDay.createMany.mock.calls
      .flatMap(([args]) => args!.data as { day: Date; key: string; avgPlayers: number; maxPlayers: number }[]);

    const keys = rows.map((row) => `${row.day.toISOString().slice(0, 10)}/${row.key}`);
    expect(new Set(keys).size).toBe(keys.length);

    const ger21 = rows.find((row) => row.key === 'GER' && row.day.toISOString().startsWith('2021-10-21'));
    expect(ger21).toMatchObject({ avgPlayers: 15, maxPlayers: 20 });
    expect(rows.some((row) => row.day.toISOString().startsWith('2021-10-23'))).toBe(false);
  });
});
