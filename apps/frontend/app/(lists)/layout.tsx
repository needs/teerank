import prisma from "@teerank/frontend/utils/prisma";
import { LayoutTabs } from "./LayoutTabs";

export default async function Index({
  children,
}: {
  children: React.ReactNode;
}) {
  const playersCount = await prisma.player.count();
  const clansCount = 1;
  const serversCount = await prisma.gameServer.count();

  return (
    <div className="flex flex-col gap-4 py-8">
      <LayoutTabs playersCount={playersCount} clansCount={clansCount} serversCount={serversCount} />

      {children}
    </div>
  )
}
