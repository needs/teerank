export function DDNetAttribution({ label }: { label?: string }) {
  return (
    <p className="text-sm text-[#999] px-4 py-2">
      {label ?? 'Official record data from'}{' '}
      <a href="https://ddnet.org" className="hover:underline" target="_blank" rel="noopener">
        DDNet.org
      </a>
    </p>
  );
}
