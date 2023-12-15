import { RankFunctionArgs } from "../tasks/rankPlayers";

export const rankPlayersTime = async ({ snapshots, getMapRating, setMapRating, markAsRanked} : RankFunctionArgs) => {
  for (const snapshot of snapshots) {
    for (const client of snapshot.clients) {
      const newTime = (!client.inGame || Math.abs(client.score) === 9999) ? undefined : -Math.abs(client.score);
      const currentTime = getMapRating(snapshot.mapId, client.playerName);

      if (newTime !== undefined && (currentTime === undefined || newTime > currentTime)) {
        setMapRating(snapshot.mapId, client.playerName, newTime);
      }
    }

    markAsRanked(snapshot.id);
  }
}
