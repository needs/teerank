export async function wait(ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms);
  });
}

export function cancellableWait(ms: number) {
  let cancel: () => void;

  const wait = new Promise((resolve) => {
    const timeoutId = setTimeout(() => resolve(void 0), ms);
    cancel = () => {
      clearTimeout(timeoutId);
      resolve(void 0);
    };
  });

  return { wait, cancel: () => cancel() };
}

export function removeDuplicatedClients<T extends { playerName: string }>(clients: T[]) {
  return clients.filter((client, index, array) => {
    return array.findIndex((otherClient) => otherClient.playerName === client.playerName) === index;
  });
}

export function toBase64(str: string) {
  return Buffer.from(str).toString('base64');
}

export function fromBase64(str: string) {
  return Buffer.from(str, 'base64').toString();
}
