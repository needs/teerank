import { notFound } from "next/navigation";
import prisma from "../../../utils/prisma";

export async function GET(
  _request: Request,
  { params }: { params: { mapId: string } }
) {
  const mapId = Number(params.mapId);

  if (!Number.isInteger(mapId)) {
    notFound();
  }

  const thumb = await prisma.ddnetMapThumb.findUnique({
    where: { mapId },
    select: { png: true },
  });

  if (thumb === null) {
    notFound();
  }

  return new Response(Buffer.from(thumb.png), {
    headers: {
      'Content-Type': 'image/png',
      'Cache-Control': 'public, max-age=86400, stale-while-revalidate=604800',
    },
  });
}
