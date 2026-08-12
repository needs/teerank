import base64url from "base64url";

// For IP encoding, replace all dots with underscores and all semicolon with
// dash to make it a valid URL path.

export function encodeIp(ip: string) {
  return ip.replace(/\./g, '_').replace(/:/g, '-');
}

export function decodeIp(ip: string) {
  return ip.replace(/_/g, '.').replace(/-/g, ':');
}

// Leave strings untouched as much as possible.  However some characters do
// break the URL even after applying encodeString.
export function encodeString(str: string) {
  if (str === '' || str.startsWith('_') || str.includes('.')) {
    return `_${base64url.encode(str)}`;
  } else {
    return encodeURIComponent(str);
  }
}

// Never throws: malformed percent-escapes fall back to the raw segment, which
// matches no record, so the page 404s instead of raising a 500.
export function decodeString(str: string) {
  try {
    if (str.startsWith('_')) {
      return base64url.decode(str.slice(1));
    } else {
      return decodeURIComponent(str);
    }
  } catch {
    return str;
  }
}
