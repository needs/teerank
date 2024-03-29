import Image from 'next/image';
import './global.css';
import Link from 'next/link';
import { HeaderTabs } from '../components/HeaderTabs';
import prisma from '../utils/prisma';
import { SearchForm } from '../components/SearchForm';
import packageJson from 'package.json';
import { GoogleAnalytics } from '@next/third-parties/google';

const gaId = process.env.NEXT_PUBLIC_GOOGLE_ANALYTICS_ID;

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
      playerCount: true,
      mapCount: true,
    },
    orderBy: [
      {
        playerCount: 'desc',
      },
      {
        mapCount: 'desc',
      },
    ],
    take: 2,
  });

  return (
    <html lang="en">
      <body className="bg-[url('/background.png')] bg-repeat-x bg-left-top flex flex-col items-center m-0 bg-[#9EB5D6] text-[#666] px-4">
        <header className="max-w-5xl w-full flex flex-col lg:flex-row relative">
          <Link
            href="https://github.com/needs/teerank"
            target="_blank"
            prefetch={false}
            className="flex flex-row bg-white rounded-b-md px-2 py-1 items-center gap-1 absolute top-0 right-0"
          >
            <Image
              src="/github/mark.png"
              alt="Github mark"
              width={16}
              height={16}
            />
            <Image
              src="/github/logo.png"
              alt="Github logo"
              height={16}
              width={1000.0 * (16.0 / 410.0)}
              style={{
                width: 'auto',
                height: '100%',
              }}
            />
          </Link>
          <Link href="/" className="py-4 grow">
            <Image
              src="/logo.png"
              alt="Logo"
              width={483}
              height={98}
              priority={true}
            />
          </Link>
          <div className="flex grow pb-4 lg:pt-4 shrink-0 lg:justify-end">
            <SearchForm />
          </div>
        </header>

        <main className="max-w-5xl w-full rounded-xl bg-[#e7e5be] shadow-sm">
          <HeaderTabs
            defaultGameTypes={defaultGameTypes.map((gameType) => gameType.name)}
          />
          <main className="bg-white rounded-xl shadow-md overflow-x-auto">
            {children}
          </main>
        </main>

        <footer className="flex flex-row gap-6 items-center py-8 text-sm text-white">
          <Link
            href="https://github.com/needs/teerank"
            prefetch={false}
            className="[text-shadow:_1px_1px_1px_#383838] hover:underline"
          >
            Teerank {packageJson.version} ({process.env.NODE_ENV})
          </Link>
          <Link
            prefetch={false}
            href="/status"
            className="[text-shadow:_1px_1px_1px_#383838] hover:underline"
          >
            Status
          </Link>
          <Link
            href="/about"
            className="[text-shadow:_1px_1px_1px_#383838] hover:underline"
          >
            About
          </Link>
        </footer>
      </body>
      {gaId !== undefined && <GoogleAnalytics gaId={gaId} />}
    </html>
  );
}
