import { LayoutTabs } from "./LayoutTabs";

export default async function Index({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <div className="flex flex-col gap-4 py-8">
      <LayoutTabs />

      {children}
    </div>
  )
}
