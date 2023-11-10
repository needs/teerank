import { List, ListCell } from "./components/List";
import { Tab, Tabs } from "./components/Tabs";

export default async function Index() {
  return (
    <div className="flex flex-col gap-4 py-8">
      <Tabs>
        <Tab label="Players" count={590000} isActive={true} />
        <Tab label="Clans" count={60000} isActive={false} />
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
        <ListCell label="1" />
        <ListCell label="Player 1" href="/" />
        <ListCell label="POLICE" href="/" />
        <ListCell label="1503" />
        <ListCell label="1 hour ago" />

        <ListCell label="2" />
        <ListCell label="Player 2" href="/" />
        <ListCell label="EVIL" href="/" />
        <ListCell label="1503" />
        <ListCell label="1 hour ago" />
      </List>
    </div>
  );
}
