import { addDefaultMasterServers } from './addDefaultMasterServers';
import { pollGameServers } from './pollGameServers';
import { pollMasterServers } from './pollMasterServers';

async function main() {
  await addDefaultMasterServers();
  await pollMasterServers();
  //await pollGameServers();
}

main();
