import { List, ListCell } from '@teerank/frontend/components/List';
import prisma from '@teerank/frontend/utils/prisma';
import { searchParamSchema } from '../schema';

export const metadata = {
  title: 'Clans',
  description: 'List of all clans',
};

export default async function Index({
  searchParams,
}: {
  searchParams: { [key: string]: string | string[] | undefined };
}) {
  return (
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
          title: 'Playtime',
          expand: true,
        },
        {
          title: 'Joined at',
          expand: false,
        },
      ]}
    >
        <>
          <ListCell alignRight label={`${1}`} />
          <ListCell label={"Test player"} href={{ pathname: `/player/${"Test"}`}} />
          <ListCell label={'5 hours'} />
          <ListCell alignRight label="363 days ago" />
        </>
    </List>
  );
}
