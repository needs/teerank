import { addDefaultMasterServers } from './addDefaultMasterServers';
import { pollMasterServers } from './pollMasterServers';

async function main() {
  await addDefaultMasterServers();
  await pollMasterServers();
}

main();
