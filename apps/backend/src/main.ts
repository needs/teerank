import { addDefaultMasterServers } from './addDefaultMasterServers';
import { pollGameServers } from './pollGameServers';
import { pollMasterServers } from './pollMasterServers';
import { rankPlayers } from './rankPlayers';
import { updateGameTypesRankMethod } from './updateGameTypesRankMethod';

async function main() {
  await addDefaultMasterServers();
  await updateGameTypesRankMethod();
  await pollMasterServers();
  await pollGameServers();
  await rankPlayers();
}

main();
