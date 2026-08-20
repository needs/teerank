import { millisecondsInHour } from "date-fns";
import { removeDuplicatedClients } from "../utils";

export const OBSERVATION_SECONDS = 5 * 60;

export type RollupSnapshot = {
  createdAt: Date;
  gameServerId: number;
  mapId: number;
  gameTypeName: string;
  numClients: number;
  clients: {
    playerName: string;
    clanName: string | null;
    inGame: boolean;
  }[];
};

export type DayRollup = {
  players: { playerName: string; playTime: number }[];
  serverDays: { gameServerId: number; avgClients: number; maxClients: number }[];
  maps: { mapId: number; playTime: number; playerCount: number }[];
  gameTypes: { gameTypeName: string; playTime: number; playerCount: number }[];
  clans: { clanName: string; playTime: number; playerCount: number }[];
};

type ServerHourAggregate = {
  gameServerId: number;
  snapshotCount: number;
  clientSum: number;
  maxClients: number;
};

type PresenceAggregate = {
  playTime: number;
  players: Set<string>;
};

function getOrCreate<K, V>(map: Map<K, V>, key: K, create: () => V): V {
  let value = map.get(key);

  if (value === undefined) {
    value = create();
    map.set(key, value);
  }

  return value;
}

function newPresence(): PresenceAggregate {
  return { playTime: 0, players: new Set() };
}

export class DayAggregator {
  private players = new Map<string, number>();
  private serverHours = new Map<string, ServerHourAggregate>();
  private maps = new Map<number, PresenceAggregate>();
  private gameTypes = new Map<string, PresenceAggregate>();
  private clans = new Map<string, PresenceAggregate>();

  addSnapshot(snapshot: RollupSnapshot) {
    const clients = removeDuplicatedClients(snapshot.clients);
    const inGameCount = clients.filter((client) => client.inGame).length;

    const hour = Math.floor(snapshot.createdAt.getTime() / millisecondsInHour);
    const serverHour = getOrCreate(this.serverHours, `${snapshot.gameServerId}\0${hour}`, () => ({
      gameServerId: snapshot.gameServerId,
      snapshotCount: 0,
      clientSum: 0,
      maxClients: 0,
    }));

    serverHour.snapshotCount += 1;
    serverHour.clientSum += snapshot.numClients;
    serverHour.maxClients = Math.max(serverHour.maxClients, snapshot.numClients);

    if (clients.length === 0) {
      return;
    }

    const map = getOrCreate(this.maps, snapshot.mapId, newPresence);
    const gameType = getOrCreate(this.gameTypes, snapshot.gameTypeName, newPresence);

    map.playTime += inGameCount * OBSERVATION_SECONDS;
    gameType.playTime += inGameCount * OBSERVATION_SECONDS;

    for (const client of clients) {
      const playTime = client.inGame ? OBSERVATION_SECONDS : 0;

      this.players.set(client.playerName, (this.players.get(client.playerName) ?? 0) + playTime);
      map.players.add(client.playerName);
      gameType.players.add(client.playerName);

      if (client.clanName !== null) {
        const clan = getOrCreate(this.clans, client.clanName, newPresence);
        clan.playTime += playTime;
        clan.players.add(client.playerName);
      }
    }
  }

  finalize(): DayRollup {
    const serverDays = new Map<number, { hourAverages: number[]; maxClients: number }>();

    for (const serverHour of this.serverHours.values()) {
      if (serverHour.clientSum === 0) {
        continue;
      }

      const serverDay = getOrCreate(serverDays, serverHour.gameServerId, () => ({
        hourAverages: [],
        maxClients: 0,
      }));

      serverDay.hourAverages.push(serverHour.clientSum / serverHour.snapshotCount);
      serverDay.maxClients = Math.max(serverDay.maxClients, serverHour.maxClients);
    }

    return {
      players: [...this.players.entries()].map(([playerName, playTime]) => ({
        playerName,
        playTime,
      })),

      serverDays: [...serverDays.entries()].map(([gameServerId, aggregate]) => ({
        gameServerId,
        avgClients: Math.round(
          aggregate.hourAverages.reduce((sum, average) => sum + average, 0) /
            aggregate.hourAverages.length
        ),
        maxClients: aggregate.maxClients,
      })),

      maps: [...this.maps.entries()].map(([mapId, aggregate]) => ({
        mapId,
        playTime: aggregate.playTime,
        playerCount: aggregate.players.size,
      })),

      gameTypes: [...this.gameTypes.entries()].map(([gameTypeName, aggregate]) => ({
        gameTypeName,
        playTime: aggregate.playTime,
        playerCount: aggregate.players.size,
      })),

      clans: [...this.clans.entries()].map(([clanName, aggregate]) => ({
        clanName,
        playTime: aggregate.playTime,
        playerCount: aggregate.players.size,
      })),
    };
  }
}
