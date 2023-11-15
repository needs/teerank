'use client';

import { Tab, Tabs } from '@teerank/frontend/components/Tabs';
import { useParams, usePathname, useSearchParams } from 'next/navigation';
import { paramsSchema, searchParamsSchema } from './schema';
import { useSearchParamsObject } from '@teerank/frontend/utils/hooks';

export function LayoutTabs() {
  const searchParams = useSearchParamsObject();
  const pathname = usePathname();
  const params = useParams();

  const { map } = searchParamsSchema.parse(searchParams);
  const { gameType } = paramsSchema.parse(params);

  const urlQuery = map === undefined ? {} : { map };
  const urlPathname = `/gametype/${encodeURIComponent(gameType)}`;

  return (
    <Tabs>
      <Tab
        label="Players"
        count={590000}
        isActive={pathname === urlPathname}
        href={{
          pathname: urlPathname,
          query: urlQuery,
        }}
      />
      <Tab
        label="Maps"
        count={60000}
        isActive={pathname === `${urlPathname}/maps`}
        href={{ pathname: `${urlPathname}/maps`, query: urlQuery }}
      />
      <Tab
        label="Servers"
        count={1200}
        isActive={pathname === `${urlPathname}/servers`}
        href={{ pathname: `${urlPathname}/servers`, query: urlQuery }}
      />
    </Tabs>
  );
}
