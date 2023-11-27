import Image from 'next/image';
import './global.css';
import Link from 'next/link';
import { HeaderTabs } from '../components/HeaderTabs';
import prisma from '../utils/prisma';

export const metadata = {
  title: 'Welcome to Teerank',
  description: 'Rank players, clans and servers from Teeworlds',
};

export default async function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  const defaultGameTypes = await prisma.gameType.findMany({
    select: {
      name: true,
      _count: {
        select: {
          playerInfos: true,
        },
      },
    },
    orderBy: [
      {
        playerInfos: {
          _count: 'desc',
        },
      },
    ],
    take: 2,
  });

  return (
    <html lang="en">
      <body className="bg-[url('/background.png')] bg-repeat-x bg-left-top flex flex-col items-center m-0 bg-[#9EB5D6] text-[#666]">
        <header className="max-w-5xl w-full flex flex-row py-4">
          <Image src="/logo.png" alt="Logo" width={483} height={98} />
        </header>

        <main className="max-w-5xl w-full rounded-xl bg-[#e7e5be] shadow-sm">
          <HeaderTabs
            defaultGameTypes={defaultGameTypes.map((gameType) => gameType.name)}
          />
          <main className="bg-white rounded-xl shadow-md">{children}</main>
        </main>

        <footer className="flex flex-row gap-6 items-center py-8 text-sm text-white">
          <Link
            href="/"
            className="[text-shadow:_1px_1px_1px_#383838] hover:underline"
          >
            Teerank 1.0 (stable)
          </Link>
          <Link
            href="/"
            className="[text-shadow:_1px_1px_1px_#383838] hover:underline"
          >
            Status
          </Link>
          <Link
            href="/"
            className="[text-shadow:_1px_1px_1px_#383838] hover:underline"
          >
            About
          </Link>
        </footer>
      </body>
    </html>
  );
}
