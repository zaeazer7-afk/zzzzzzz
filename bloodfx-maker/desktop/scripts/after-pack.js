// Puts the app icon and version details into BloodFX Maker.exe. electron-builder's own step for this needs Wine
// when building on Linux, so it is turned off (signAndEditExecutable: false) and done here in plain JavaScript.
const fs = require('fs');
const path = require('path');

exports.default = async function afterPack(context) {
  if (context.electronPlatformName !== 'win32') return;
  const { NtExecutable, NtExecutableResource, Data, Resource } = await import('resedit');
  const info = context.packager.appInfo;
  const exe = path.join(context.appOutDir, `${info.productFilename}.exe`);
  const pe = NtExecutable.from(fs.readFileSync(exe), { ignoreCert: true });
  const res = NtExecutableResource.from(pe);

  const icon = Data.IconFile.from(fs.readFileSync(path.join(context.packager.info.buildResourcesDir, 'icon.ico')));
  const group = Resource.IconGroupEntry.fromEntries(res.entries)[0];
  Resource.IconGroupEntry.replaceIconsForResource(res.entries, group ? group.id : 1, group ? group.lang : 1033, icon.icons.map((i) => i.data));

  const [major, minor, patch] = info.version.split('.').map((n) => parseInt(n, 10) || 0);
  const version = Resource.VersionInfo.fromEntries(res.entries)[0] || Resource.VersionInfo.createEmpty();
  version.setFileVersion(major, minor, patch, 0);
  version.setProductVersion(major, minor, patch, 0);
  const languages = version.getAllLanguagesForStringValues();
  for (const lang of languages.length ? languages : [{ lang: 1033, codepage: 1200 }]) {
    version.setStringValues(lang, {
      ProductName: info.productName,
      FileDescription: info.productName,
      CompanyName: info.companyName || info.productName,
      LegalCopyright: info.copyright || '',
      OriginalFilename: `${info.productFilename}.exe`,
      InternalName: info.productFilename,
      FileVersion: info.version,
      ProductVersion: info.version,
    });
  }
  version.outputToResourceEntries(res.entries);
  res.outputResource(pe);
  fs.writeFileSync(exe, Buffer.from(pe.generate()));
  console.log(`  • set icon and version ${info.version} on ${path.basename(exe)}`);
};
