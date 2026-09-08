import { Readable } from "stream";
import { parseCsvEntry } from "./stream";

describe('parseCsvEntry', () => {
  it('decodes the MySQL-style backslash escapes used by the dump', async () => {
    const csv = [
      '"Map","Name","Time"',
      '"For Idiots 1","Tobias\\"",514.2',
      '"Bootcamp #2","I\\\\I4I\\\\/I3",808.46',
      '"Lowcore","-/Noob\\\\-Fra",167.14',
    ].join('\n') + '\n';

    const records: string[][] = [];
    await parseCsvEntry(Readable.from([csv]), (record) => {
      records.push(record);
    });

    expect(records).toEqual([
      ['For Idiots 1', 'Tobias"', '514.2'],
      ['Bootcamp #2', 'I\\I4I\\/I3', '808.46'],
      ['Lowcore', '-/Noob\\-Fra', '167.14'],
    ]);
  });
});
