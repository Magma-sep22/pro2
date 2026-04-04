module.exports = {
  packagerConfig: {
    asar: true,
    // icon: "./build/Parachute", // ปิดไว้ก่อนเพราะยังไม่มีไฟล์
  },
  rebuildConfig: {},
  makers: [
    {
      // เปิดใช้งานส่วนสร้างไฟล์ Setup ของ Windows
      name: "@electron-forge/maker-squirrel",
      config: {
        authors: "EdgeFlyte",
        description: "EF CubeSat GTerminal V1",
        // setupIcon: "./build/Parachute.ico", // ปิดไว้ก่อนเพราะยังไม่มีไฟล์
        exe: "EFCSGTerm.exe",
        name: "EFCSGTerm",
      },
    },
    {
      name: "@electron-forge/maker-zip",
      platforms: ["darwin", "linux", "win32"],
    },
    {
      name: "@electron-forge/maker-deb",
      config: {
        options: {
          maintainer: "EdgeFlyte",
          homepage: "https://edgeflyte.com",
        },
      },
    },
    {
      name: "@electron-forge/maker-rpm",
      config: {
        options: {
          license: "MIT",
          description: "EF CubeSat GTerminal V1",
        },
      },
    }
  ]
};