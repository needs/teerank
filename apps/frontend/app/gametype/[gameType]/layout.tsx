import prisma from "@teerank/frontend/utils/prisma";
import { LayoutTabs } from "./LayoutTabs";
import { searchParamsSchema } from "./schema";

export default async function Index({
  children,
  params,
  searchParams,
}: {
  children: React.ReactNode;
  params: { gameType: string };
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  const parsedSearchParams = searchParamsSchema.parse(searchParams);
  const gameType = decodeURIComponent(params.gameType);

  const where =
    parsedSearchParams.map === undefined
      ? {
          gameType: {
            name: { equals: gameType },
          },
        }
      : {
          map: {
            name: { equals: parsedSearchParams.map },
          },
        };

  const playerCount = await prisma.playerInfo.count({
    where,
  });

  const mapConditional =
    parsedSearchParams.map === undefined
      ? {}
      : {
          name: { equals: parsedSearchParams.map },
        };

  const gameTypeConditional = {
    gameTypeName: { equals: gameType },
  };

  const serverCount = await prisma.gameServer.count({
    where: {
      AND: [
        { lastSnapshot: { isNot: null } },
        {
          lastSnapshot: { map: { ...mapConditional, ...gameTypeConditional } },
        },
      ],
    },
  });

  return (
    <div className="flex flex-col gap-4 py-8">
      <LayoutTabs playerCount={playerCount} serverCount={serverCount}/>

      {children}
    </div>
  )
}
