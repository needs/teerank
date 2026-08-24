import { millisecondsInHour } from "date-fns";
import { getEnvInt } from "@teerank/teerank";
import { removeDuplicatedClients } from "../utils";

export const OBSERVATION_SECONDS = 5 * 60;

export const PARTNER_DAY_THRESHOLD_SECONDS = getEnvInt('PARTNER_DAY_THRESHOLD_SECONDS', 3600);

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
  players: {
    playerName: string;
    playTime: number;
    pollCount: number;
    occurrenceCount: number;
  }[];
  serverDays: { gameServerId: number; avgClients: number; maxClients: number }[];
  maps: { mapId: number; playTime: number; playerCount: number }[];
  gameTypes: { gameTypeName: string; playTime: number; playerCount: number }[];
  clans: { clanName: string; playTime: number; playerCount: number }[];
  partners: { playerName: string; partnerName: string; playTime: number }[];
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

type PlayerAggregate = {
  playTime: number;
  occurrenceCount: number;
  polls: Set<number>;
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

function newPlayerAggregate(): PlayerAggregate {
  return { playTime: 0, occurrenceCount: 0, polls: new Set() };
}

export class DayAggregator {
  private players = new Map<string, PlayerAggregate>();
  private serverHours = new Map<string, ServerHourAggregate>();
  private maps = new Map<number, PresenceAggregate>();
  private gameTypes = new Map<string, PresenceAggregate>();
  private clans = new Map<string, PresenceAggregate>();
  private partners = new Map<string, number>();

  constructor(private partnerThresholdSeconds = PARTNER_DAY_THRESHOLD_SECONDS) {}

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

    // Snapshots taken in the same observation window count as one poll, so a
    // name on several servers at once has more occurrences than polls.
    const poll = Math.floor(snapshot.createdAt.getTime() / (OBSERVATION_SECONDS * 1000));

    for (const client of clients) {
      const playTime = client.inGame ? OBSERVATION_SECONDS : 0;

      const player = getOrCreate(this.players, client.playerName, newPlayerAggregate);
      player.playTime += playTime;
      player.occurrenceCount += 1;
      player.polls.add(poll);

      map.players.add(client.playerName);
      gameType.players.add(client.playerName);

      if (client.clanName !== null) {
        const clan = getOrCreate(this.clans, client.clanName, newPresence);
        clan.playTime += playTime;
        clan.players.add(client.playerName);
      }
    }

    const inGameNames = clients
      .filter((client) => client.inGame)
      .map((client) => client.playerName)
      .sort();

    for (let i = 0; i < inGameNames.length; i++) {
      for (let j = i + 1; j < inGameNames.length; j++) {
        const key = `${inGameNames[i]}\0${inGameNames[j]}`;
        this.partners.set(key, (this.partners.get(key) ?? 0) + OBSERVATION_SECONDS);
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
      players: [...this.players.entries()].map(([playerName, aggregate]) => ({
        playerName,
        playTime: aggregate.playTime,
        pollCount: aggregate.polls.size,
        occurrenceCount: aggregate.occurrenceCount,
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

      partners: [...this.partners.entries()]
        .filter(([, playTime]) => playTime >= this.partnerThresholdSeconds)
        .map(([key, playTime]) => {
          const [playerName, partnerName] = key.split('\0');
          return { playerName, partnerName, playTime };
        }),
    };
  }
}
