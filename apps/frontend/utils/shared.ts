export function sharedHiddenParam(searchParams: {
  [key: string]: string | string[] | undefined;
}) {
  return searchParams['shared'] === 'hidden';
}
