'use client';

import Link from 'next/link';
import { useParams, usePathname, useSearchParams } from 'next/navigation';
import { twMerge } from 'tailwind-merge';
import { UrlObject } from 'url';
import { paramsSchema as paramsSchemaGameType } from '../app/gametype/[gameType]/schema';
import { paramsSchema as paramsSchemaServer } from '../app/server/[ip]/[port]/schema';
import { paramsSchema as paramsSchemaPlayer } from '../app/player/[playerName]/schema';

function HeaderTab({
  label,
  url,
  isActive,
  sublabel,
  sublabelPlaceholder,
}: {
  label: string;
  url: UrlObject;
  isActive: boolean;
  sublabel?: string;
  sublabelPlaceholder?: string;
}) {
  const content = (
    <div
      className={twMerge(
        'flex flex-row divide-x py-1.5 rounded-t-md border border-[#dddddd] border-b-0 text-base divide-[#999]',
        isActive ? 'bg-white' : 'bg-gradient-to-b from-[#ffffff] to-[#cccccc]'
      )}
    >
      <span className="px-4">{label}</span>
      {sublabel !== undefined && <span className="px-4">{sublabel}</span>}
      {sublabel === undefined && sublabelPlaceholder !== undefined && (
        <span className="px-4 italic">{sublabelPlaceholder}</span>
      )}
    </div>
  );

  return isActive ? content : <Link href={url}>{content}</Link>;
}

function HeaderTabSeparator() {
  return <div className="grow" />;
}

function HeaderTabGameType({ gameType }: { gameType: string }) {
  const searchParams = useSearchParams();
  const params = useParams();

  const queryGameType = decodeURIComponent(params['gameType']);
  const queryMap = searchParams.get('map');

  const isActive = queryGameType === gameType;

  return (
    <HeaderTab
      label={gameType}
      url={{ pathname: `/gametype/${encodeURIComponent(gameType)}` }}
      isActive={isActive}
      sublabel={isActive && queryMap !== null ? queryMap : undefined}
      sublabelPlaceholder={isActive ? 'All maps' : undefined}
    />
  );
}

function HeaderTabPage({
  label,
  pathname,
  extraPathnames,
}: {
  label: string;
  pathname: string;
  extraPathnames?: string[];
}) {
  const urlPathname = usePathname();
  const isActive =
    pathname === urlPathname ||
    (extraPathnames !== undefined && extraPathnames.includes(urlPathname));

  return <HeaderTab label={label} url={{ pathname }} isActive={isActive} />;
}

export function HeaderTabs() {
  const defaultGameTypes = ['CTF', 'DM'];

  const params = useParams();

  const parsedParams = paramsSchemaGameType
    .merge(paramsSchemaServer)
    .merge(paramsSchemaPlayer)
    .partial()
    .parse(params);

  return (
    <header className="flex flex-row gap-2 -mt-1.5 px-8">
      <HeaderTabPage
        label="Home"
        pathname="/"
        extraPathnames={['/clans', '/servers']}
      />

      {defaultGameTypes.map((gameType) => (
        <HeaderTabGameType key={gameType} gameType={gameType} />
      ))}

      {parsedParams.gameType !== undefined &&
        !defaultGameTypes.includes(parsedParams.gameType) && (
          <HeaderTabGameType gameType={parsedParams.gameType} />
        )}

      <HeaderTabPage label="..." pathname="/gametypes" />
      <HeaderTabSeparator />

      {parsedParams.ip !== undefined && parsedParams.port !== undefined && (
        <HeaderTabPage
          label="Server"
          pathname={`/server/${parsedParams.ip}/${parsedParams.port}`}
        />
      )}

      {parsedParams.playerName !== undefined && (
        <HeaderTabPage
          label="Player"
          pathname={`/player/${encodeURIComponent(parsedParams.playerName)}`}
        />
      )}

      <HeaderTabPage label="About" pathname="/about" />
    </header>
  );
}
