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
import { encodeIp, encodeString } from '../utils/encoding';
import { ReactNode } from 'react';

function HeaderTab({
  label,
  icon,
  url,
  isActive,
  sublabel,
  sublabelPlaceholder,
  sublabelHref,
  labelHref,
  hideOnTabletAndMobile,
}: {
  label: string;
  icon?: ReactNode;
  url: UrlObject;
  isActive: boolean;
  sublabel?: string;
  sublabelPlaceholder?: string;
  sublabelHref?: UrlObject;
  labelHref?: UrlObject;
  hideOnTabletAndMobile?: boolean;
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
        <Link href={labelHref} className="flex flex-row gap-2 px-4 hover:underline">
          {icon}
          {label}
        </Link>
      );
    } else {
      return <span className="flex flex-row gap-2 px-4">{icon}{label}</span>;
    }
  };

  const content = (
    <div
      className={twMerge(
        'flex flex-row divide-x py-1.5 rounded-t-md border border-[#dddddd] border-b-0 text-base divide-[#999] shrink-0',
        isActive ? 'bg-white' : 'bg-gradient-to-b from-[#ffffff] to-[#cccccc]',
        hideOnTabletAndMobile === true && 'hidden lg:flex'
      )}
    >
      {labelComponent()}
      {sublabelComponent()}
    </div>
  );

  return isActive ? content : <Link href={url} className='shrink-0'>{content}</Link>;
}

function HeaderTabSeparator() {
  return <div className="grow" />;
}

function HeaderTabGameType({
  gameTypeName,
  hideOnTabletAndMobile,
}: {
  gameTypeName: string;
  hideOnTabletAndMobile?: boolean;
}) {
  const params = useParams();

  const parsedParams = paramsSchemaGameType
    .merge(paramsSchemaMap)
    .partial()
    .parse(params);

  const isActive = parsedParams.gameTypeName === gameTypeName;

  return (
    <HeaderTab
      label={gameTypeName}
      url={{ pathname: `/gametype/${encodeString(gameTypeName)}` }}
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
          : { pathname: `/gametype/${encodeString(gameTypeName)}` }
      }
      sublabelHref={{
        pathname: `/gametype/${encodeString(gameTypeName)}/maps`,
      }}
      hideOnTabletAndMobile={hideOnTabletAndMobile}
    />
  );
}

function HeaderTabPage({
  label,
  icon,
  pathname,
  extraPathnames,
}: {
  label: string;
  icon?: ReactNode;
  pathname: string;
  extraPathnames?: string[];
}) {
  const urlPathname = usePathname();
  const isActive =
    pathname === urlPathname ||
    (extraPathnames !== undefined && extraPathnames.includes(urlPathname));

  return <HeaderTab label={label} url={{ pathname }} isActive={isActive} icon={icon} />;
}

export function HeaderTabs({
  defaultGameTypes,
}: {
  defaultGameTypes: string[];
}) {
  const params = useParams();
  const pathname = usePathname();

  const schema = paramsSchemaGameType
    .merge(paramsSchemaServer)
    .merge(paramsSchemaPlayer)
    .merge(paramsSchemaMap)
    .merge(paramsSchemaClan)
    .partial();

  const parsedParams = schema.safeParse(params);

  const { ip, port, gameTypeName, playerName, clanName } = parsedParams.success
    ? parsedParams.data
    : schema.parse({});

  return (
    <header className="flex flex-row gap-2 -mt-1.5 px-2 md:px-4 lg:px-8 overflow-x-auto">
      <HeaderTabPage
        label="Home"
        icon={<img src="/home.svg" />}
        pathname="/"
      />

      <HeaderTabPage
        label="Rankings"
        icon={<img src="/ranking.png" />}
        pathname="/all"
        extraPathnames={['/all/clans', '/all/servers']}
      />

      {defaultGameTypes.map((gameType) => (
        <HeaderTabGameType
          key={gameType}
          gameTypeName={gameType}
          hideOnTabletAndMobile={gameType !== gameTypeName}
        />
      ))}

      {gameTypeName !== undefined &&
        !defaultGameTypes.includes(gameTypeName) && (
          <HeaderTabGameType gameTypeName={gameTypeName} />
        )}

      <HeaderTabPage label="..." pathname="/gametypes" />
      <HeaderTabSeparator />

      {ip !== undefined && port !== undefined && (
        <HeaderTabPage
          label="Server"
          pathname={`/server/${encodeIp(ip)}/${port}`}
        />
      )}

      {playerName !== undefined && (
        <HeaderTabPage
          label="Player"
          pathname={`/player/${encodeString(playerName)}`}
          extraPathnames={[
            `/player/${encodeString(playerName)}/maps`,
            `/player/${encodeString(playerName)}/clans`,
          ]}
        />
      )}

      {clanName !== undefined && (
        <HeaderTabPage
          label="Clan"
          pathname={`/clan/${encodeString(clanName)}`}
          extraPathnames={[
            `/clan/${encodeString(clanName)}/gametypes`,
            `/clan/${encodeString(clanName)}/maps`,
          ]}
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
        <HeaderTabPage label="Status" pathname="/status" />
      )}

      <HeaderTabPage label="About" pathname="/about" />
    </header>
  );
}
