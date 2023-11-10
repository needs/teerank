import { List, ListCell } from '../components/List';
import { Tab, Tabs } from '../components/Tabs';
import prisma from '../utils/prisma';
import { z } from 'zod';

const searchParamSchema = z
  .object({
    gametype: z.string().optional(),
    map: z.string().optional(),
    page: z.coerce.number().optional().default(1),
  })
  .catch({ page: 1 });

export default async function Index({
  searchParam,
}: {
  searchParam: { [key: string]: string | string[] | undefined };
}) {
  const validatedSearchParam = searchParamSchema.parse(searchParam);

  const mapConditional = {
    name: { equals: validatedSearchParam.map ?? null },
  };

  const gametypeConditional =
    validatedSearchParam.gametype === undefined
      ? {}
      : {
          gameTypeName: { equals: validatedSearchParam.gametype },
        };

  const ratings = await prisma.playerRating.findMany({
    where: {
      map: {
        ...mapConditional,
        ...gametypeConditional,
      },
    },
    orderBy: {
      rating: 'desc',
    },
    include: {
      player: true,
    },
    take: 100,
    skip: (validatedSearchParam.page - 1) * 100,
  });

  console.log("ratings", ratings);
  console.log("mapConditional", mapConditional);
  console.log("gametypeConditional", gametypeConditional);

  return (
    <div className="flex flex-col gap-4 py-8">
      <Tabs>
        <Tab label="Players" count={590000} isActive={true} />
        <Tab label="Maps" count={60000} isActive={false} />
        <Tab label="Servers" count={1200} isActive={false} />
      </Tabs>

      <List
        columns={[
          {
            title: '',
            expand: false,
          },
          {
            title: 'Name',
            expand: true,
          },
          {
            title: 'Clan',
            expand: true,
          },
          {
            title: 'Elo',
            expand: false,
          },
          {
            title: 'Last Seen',
            expand: false,
          },
        ]}
      >
        {ratings.map((rating, index) => (
          <>
            <ListCell alignRight label={`${index + 1}`} />
            <ListCell
              label={rating.player.name}
              href={`/players/${rating.player.name}`}
            />
            <ListCell label={''} />
            <ListCell
              alignRight
              label={`${Intl.NumberFormat('en-US', {
                maximumFractionDigits: 0,
              }).format(rating.rating)}`}
            />
            <ListCell alignRight label="1 hour ago" />
          </>
        ))}
      </List>
    </div>
  );
}
