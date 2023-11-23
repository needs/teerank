import prisma from "@teerank/frontend/utils/prisma";
import { LayoutTabs } from "./LayoutTabs";

export default async function Index({
  children,
}: {
  children: React.ReactNode;
}) {
  const counts = await Promise.all([
    await prisma.player.count(),
    await prisma.clan.count(),
    await prisma.gameServer.count(),
  ]);

  return (
    <div className="flex flex-col gap-4 py-8">
      <LayoutTabs playerCount={counts[0]} clanCount={counts[1]} serverCount={counts[2]} />

      {children}
    </div>
  )
}
