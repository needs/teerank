'use client';

import Link from 'next/link';
import { useParams, usePathname, useSearchParams } from 'next/navigation';
import { twMerge } from 'tailwind-merge';
import { UrlObject } from 'url';
import { paramsSchema as paramsSchemaMap } from '../app/gametype/[gameTypeName]/map/[mapName]/schema';
import { paramsSchema as paramsSchemaGameType } from '../app/gametype/[gameTypeName]/schema';
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

function HeaderTabGameType({ gameTypeName }: { gameTypeName: string }) {
  const params = useParams();

  const parsedParams = paramsSchemaGameType
    .merge(paramsSchemaMap)
    .partial()
    .parse(params);

  const isActive = parsedParams.gameTypeName === gameTypeName;

  return (
    <HeaderTab
      label={gameTypeName}
      url={{ pathname: `/gametype/${encodeURIComponent(gameTypeName)}` }}
      isActive={isActive}
      sublabel={
        isActive && parsedParams.mapName !== undefined
          ? parsedParams.mapName
          : undefined
      }
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

  const { ip, port, gameTypeName, mapName, playerName } = paramsSchemaGameType
    .merge(paramsSchemaServer)
    .merge(paramsSchemaPlayer)
    .merge(paramsSchemaMap)
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
        <HeaderTabGameType key={gameType} gameTypeName={gameType} />
      ))}

      {gameTypeName !== undefined &&
        !defaultGameTypes.includes(gameTypeName) && (
          <HeaderTabGameType gameTypeName={gameTypeName} />
        )}

      <HeaderTabPage label="..." pathname="/gametypes" />
      <HeaderTabSeparator />

      {ip !== undefined && port !== undefined && (
        <HeaderTabPage label="Server" pathname={`/server/${ip}/${port}`} />
      )}

      {playerName !== undefined && (
        <HeaderTabPage
          label="Player"
          pathname={`/player/${encodeURIComponent(playerName)}`}
        />
      )}

      <HeaderTabPage label="About" pathname="/about" />
    </header>
  );
}
