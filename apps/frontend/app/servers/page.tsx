import { permanentRedirect } from "next/navigation";

export default async function Index() {
  permanentRedirect('/all/servers')
}
