# Q-SYS Design Property Handler

Shows which version of Q-SYS Designer saved a `.qsys` file, right in Windows
Explorer, without opening it.

| Property | Example |
| --- | --- |
| **Comments** | `Q-SYS Designer 9.10.2` |
| **Product version** | `9.10.2` |

The version appears in the Details pane (Alt+Shift+P), the hover tooltip, file
Properties > Details, and as sortable columns in Details view. It also works in
Open/Save dialogs and Windows Search. The properties are read-only.

It works with every Designer release it has been tested against, from 5.2 to 10.5. It doesn't depend on which Designer
versions are installed, and it reads only the first 1 KB of each file, so it
stays fast on large designs and network shares.

## Install

1. Download `QsysPropertyHandler-<version>-x64.zip` from the release and unzip it anywhere.
2. Run `install.cmd`. It asks for administrator rights, copies the handler to
   `C:\Program Files\Q-SYS Design Property Handler\`, registers it, and offers
   to restart Explorer.

To upgrade, run the new release's `install.cmd`. You don't need to uninstall
first.

### Add the columns

Explorer doesn't let you set columns for "a folder and everything under it".
It saves a view either for one folder or for a **folder template**: General
items, Documents, Pictures, Music or Videos. You can apply a view to every
folder that uses a template. So to cover a whole tree, you control which
template the tree uses.

#### One folder

1. Open the folder and switch to **View > Details**.
2. Right-click any column header > **More...**, tick **Comments** and/or
   **Product version**, then click OK. Drag the new columns to where you want
   them.

#### Globally (every folder)

1. Set up the columns in one folder, as above. That folder's template should
   be **General items**: right-click the folder > Properties > Customize.
   Template folders such as Downloads use their own templates.
2. **View > Options > View** tab > **Apply to Folders** > Yes.

This copies the folder's whole Details layout, including column order, widths
and sorting, to every folder with the same template. Folders you have
customised yourself keep their own view. To include them, click **Reset
Folders** on the same tab first, then **Apply to Folders**.

#### Only a project tree (a folder and all its subfolders)

Give the tree a template you don't use anywhere else, then apply the view to
that template only. **Documents** works well, because it is a general-purpose
template.

1. Right-click the top folder (for example `C:\Projects`) > **Properties >
   Customize**.
2. Set **Optimize this folder for** to **Documents**, tick **Also apply this
   template to all subfolders**, then click OK.
3. Open the top folder and add the columns, as in *One folder*.
4. **View > Options > View** tab > **Apply to Folders** > Yes.

Every folder in the tree now shows the columns. Folders outside it keep their
layout, as long as they use a different template. Apply to Folders affects
every folder with the Documents template, including your Documents library.
If that is a problem, pick the template you use least.

New subfolders made later start with the parent's template, so they get the
columns too. A folder copied in from elsewhere keeps its own template. Re-run
step 2 on the top folder to bring it in line.

#### If the columns don't stick

- Under **View > Options > View**, make sure **Remember each folder's view
  settings** is ticked.
- Explorer remembers views for a limited number of folders (5,000 by
  default). On very large trees, older folders fall back to their template
  view. Applying the view to the template, as above, covers them.
- To start over: **View > Options > View > Reset Folders**.

## Uninstall

Run `uninstall.cmd`, either from the unzipped release or from
`C:\Program Files\Q-SYS Design Property Handler\`. Let it restart Explorer so
the DLL is released and the folder can be removed. If something else still holds
the DLL, sign out and back in, then delete the folder.

## Command line

`qsysver.exe` prints the version of each file you pass it:

```
> qsysver "Board Room.qsys" "Lobby.qsys"
10.5.0	Board Room.qsys
9.13.1	Lobby.qsys
```

It exits with 1 if any file isn't a readable `.qsys` file.

## Requirements and limits

- Windows 10 or 11, x64. 32-bit programs' file dialogs won't show the values.
- The DLL isn't code-signed. Windows SmartScreen may warn about the scripts in
  a downloaded zip: right-click the zip > Properties > **Unblock** before you
  unzip it.
- The handler only adds properties. It doesn't change Designer's file
  association, icon, or how `.qsys` files open.
- The handler takes over Comments for `.qsys` files. Comments can't be typed in
  by hand for these files, but Windows never supported that for `.qsys` anyway.

## Troubleshooting

- **Nothing shows after install.** Restart Explorer (`install.cmd` offers to),
  or sign out and back in.
- **Blank for one file.** The file isn't a Designer design, or its header is
  damaged. `qsysver.exe` on that file will say "not a .qsys header".
- **Check the registration:**
  `reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\PropertySystem\PropertyHandlers\.qsys"`
  should print `{6F1E4B7A-2C3D-4E8F-9A1B-5C7D3E2F8A91}`.

## How it works

A `.qsys` file starts with a small, uncompressed .NET BinaryFormatter record,
`QSC.QSys.Design.FileIO+FileHeader`, followed by the GZip-compressed design.
The header's `_Version` field is a `System.Version`, which holds the version of
Designer that saved the file. The handler finds that field and reads its four
little-endian Int32s; see [`src/qsys_version.h`](src/qsys_version.h). Revisions
of 0 or -1, which Designer 9.13 and later write, are left off.

The handler is a native in-process COM server. It implements
`IInitializeWithStream`, `IPropertyStore` and `IPropertyStoreCapabilities`.
Registration writes these keys:

- `HKLM\Software\Classes\CLSID\{6F1E4B7A-...}`: the COM server.
- `HKLM\...\PropertySystem\PropertyHandlers\.qsys`: binds the handler to the extension.
- `HKLM\Software\Classes\SystemFileAssociations\.qsys`: `FullDetails`,
  `PreviewDetails` and `InfoTip`, the lists of properties Explorer shows. These
  go here rather than on Designer's ProgID, because the ProgID's name changes
  with each Designer version.

## Building from source

You need Visual Studio 2022 or its Build Tools, with the
**Desktop development with C++** workload:

```
winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

| Command | What it does |
| --- | --- |
| `build.cmd` | Builds `bin\QsysPropertyHandler.dll` and `bin\qsysver.exe` (x64, static CRT, no runtime needed). |
| `powershell -File tests\run-tests.ps1` | Checks the parser against the header fixtures. Add `-Shell` to check the registered handler through Explorer's property system. |
| `package.cmd` | Builds, tests and writes `dist\QsysPropertyHandler-<version>-x64.zip` plus a `.sha256`. |
| `install.cmd` / `uninstall.cmd` | Install from `bin\` when run in the source tree. |

`build.cmd` works while the handler is loaded in Explorer: it moves the locked
DLL aside into `obj\`. Explorer keeps using the old copy until it restarts.

The test fixtures in `tests\fixtures\valid` contain only file headers, with no
design content. Each file is named after the version it should parse to.

### Releasing

1. Bump `QPH_VERSION_*` in [`src/version.h`](src/version.h).
2. Add an entry to [`CHANGELOG.md`](CHANGELOG.md).
3. Run `package.cmd`, then `install.cmd` and
   `powershell -File tests\run-tests.ps1 -Shell`.
4. Tag `v<version>`, then publish the zip and its `.sha256`.

## License

MIT. See [LICENSE](LICENSE). Q-SYS is a trademark of QSC, LLC. This project is
not affiliated with or endorsed by QSC.
