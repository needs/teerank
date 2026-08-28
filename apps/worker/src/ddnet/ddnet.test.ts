import { BoundaryTracker, monthLabel, raceRowKey } from "./state";
import { groupTeamRows } from "./applyChunk";
import { parseMapperNames } from "./mapsImport";
import { ddnetMapFileName } from "./stream";

describe('parseMapperNames', () => {
  it('splits multi-mapper conventions', () => {
    expect(parseMapperNames('Ravie')).toEqual(['Ravie']);
    expect(parseMapperNames('Pipou & Ravie')).toEqual(['Pipou', 'Ravie']);
    expect(parseMapperNames('A, B & C')).toEqual(['A', 'B', 'C']);
    expect(parseMapperNames('')).toEqual([]);
  });
});

describe('ddnetMapFileName', () => {
  it('replaces every non-alphanumeric character with underscores', () => {
    expect(ddnetMapFileName('Kobra 4')).toBe('Kobra_4');
    expect(ddnetMapFileName('#wontfix')).toBe('_wontfix');
    expect(ddnetMapFileName('Aim 10.0')).toBe('Aim_10_0');
    expect(ddnetMapFileName('-273')).toBe('_273');
  });
});

describe('BoundaryTracker', () => {
  it('keeps only keys within the window of the max timestamp', () => {
    const tracker = new BoundaryTracker();
    tracker.add('old', new Date('2024-01-01T00:00:00Z'));
    tracker.add('near', new Date('2024-01-02T23:55:00Z'));
    tracker.add('max', new Date('2024-01-03T00:00:00Z'));

    const { watermark, boundary } = tracker.finalize();
    expect(watermark).toBe('2024-01-03T00:00:00.000Z');
    expect(boundary.sort()).toEqual(['max', 'near']);
  });
});

describe('groupTeamRows', () => {
  it('groups rows by team id keeping the roster', () => {
    const timestamp = new Date('2020-05-01T12:00:00Z');
    const teams = groupTeamRows([
      { teamId: 'aa', mapName: 'Kobra', playerName: 'a', time: 100, timestamp },
      { teamId: 'aa', mapName: 'Kobra', playerName: 'b', time: 100, timestamp },
      { teamId: 'bb', mapName: 'Just8', playerName: 'c', time: 50, timestamp },
    ]);

    expect(teams).toHaveLength(2);
    expect(teams[0]).toMatchObject({ teamId: 'aa', playerNames: ['a', 'b'] });
    expect(teams[1]).toMatchObject({ teamId: 'bb', playerNames: ['c'] });
  });
});

describe('raceRowKey / monthLabel', () => {
  it('builds stable keys', () => {
    const timestamp = new Date('2019-07-28T16:36:11Z');
    expect(raceRowKey({ mapName: 'Kobra', playerName: 'deen', timestamp, time: 12.34 }))
      .toBe('Kobra|deen|2019-07-28T16:36:11.000Z|12.34');
    expect(monthLabel(timestamp)).toBe('2019-07');
  });
});
