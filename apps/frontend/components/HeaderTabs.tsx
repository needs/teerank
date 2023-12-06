'use client';

import Link from 'next/link';
import { useParams, usePathname } from 'next/navigation';
import { twMerge } from 'tailwind-merge';
import { UrlObject } from 'url';
import { paramsSchema as paramsSchemaMap } from '../app/gametype/[gameTypeName]/map/[mapName]/schema';
import { paramsSchema as paramsSchemaGameType } from '../app/gametype/[gameTypeName]/schema';
import { paramsSchema as paramsSchemaServer } from '../app/server/[ip]/[port]/schema';
import { paramsSchema as paramsSchemaPlayer } from '../app/player/[playerName]/schema';
import { paramsSchema as paramsSchemaClan } from '../app/clan/[clanName]/schema';

function HeaderTab({
  label,
  url,
  isActive,
  sublabel,
  sublabelPlaceholder,
  sublabelHref,
  labelHref,
}: {
  label: string;
  url: UrlObject;
  isActive: boolean;
  sublabel?: string;
  sublabelPlaceholder?: string;
  sublabelHref?: UrlObject;
  labelHref?: UrlObject;
}) {
  const sublabelComponent = () => {
    if (sublabel === undefined) {
      if (sublabelPlaceholder !== undefined) {
        if (sublabelHref !== undefined) {
          return (
            <Link className="px-4 italic hover:underline" href={sublabelHref}>
              {sublabelPlaceholder}
            </Link>
          );
        } else {
          return <span className="px-4 italic">{sublabelPlaceholder}</span>;
        }
      }
    } else {
      if (sublabelHref !== undefined) {
        return (
          <Link className="px-4 hover:underline" href={sublabelHref}>
            {sublabel}
          </Link>
        );
      } else {
        return <span className="px-4">{sublabel}</span>;
      }
    }

    return null;
  };

  const labelComponent = () => {
    if (labelHref !== undefined) {
      return (
        <Link href={labelHref} className="px-4 hover:underline">
          {label}
        </Link>
      );
    } else {
      return <span className="px-4">{label}</span>;
    }
  };

  const content = (
    <div
      className={twMerge(
        'flex flex-row divide-x py-1.5 rounded-t-md border border-[#dddddd] border-b-0 text-base divide-[#999]',
        isActive ? 'bg-white' : 'bg-gradient-to-b from-[#ffffff] to-[#cccccc]'
      )}
    >
      {labelComponent()}
      {sublabelComponent()}
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
      labelHref={
        !isActive
          ? undefined
          : { pathname: `/gametype/${encodeURIComponent(gameTypeName)}` }
      }
      sublabelHref={{
        pathname: `/gametype/${encodeURIComponent(gameTypeName)}/maps`,
      }}
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

export function HeaderTabs({
  defaultGameTypes,
}: {
  defaultGameTypes: string[];
}) {
  const params = useParams();
  const pathname = usePathname();

  const { ip, port, gameTypeName, playerName, clanName } = paramsSchemaGameType
    .merge(paramsSchemaServer)
    .merge(paramsSchemaPlayer)
    .merge(paramsSchemaMap)
    .merge(paramsSchemaClan)
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

      {clanName !== undefined && (
        <HeaderTabPage
          label="Clan"
          pathname={`/clan/${encodeURIComponent(clanName)}`}
          extraPathnames={[`/clan/${encodeURIComponent(clanName)}/gametypes`, `/clan/${encodeURIComponent(clanName)}/maps`]}
        />
      )}

      {['/search', '/search/clans', '/search/servers'].includes(pathname) && (
        <HeaderTabPage
          label="Search"
          pathname="/search"
          extraPathnames={['/search/clans', '/search/servers']}
        />
      )}

      {['/status'].includes(pathname) && (
        <HeaderTabPage
          label="Status"
          pathname="/status"
        />
      )}

      <HeaderTabPage label="About" pathname="/about" />
    </header>
  );
}
