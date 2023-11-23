import { Prisma } from "@prisma/client";

export async function wait(ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms);
  });
}

export function removeDuplicatedClients(clients: Prisma.GameServerClientGetPayload<true>[]) {
  return clients.filter((client, index, array) => {
    return array.findIndex((otherClient) => otherClient.playerName === client.playerName) === index;
  });
}
