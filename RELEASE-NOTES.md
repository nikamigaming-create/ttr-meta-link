# TTR Meta Link 1.0.0 beta

A separate MO2 compatibility plugin for running the supported TTWVR build through
Meta Link / Air Link and Meta's 32-bit OpenXR runtime. SteamVR and Virtual Desktop
are not required for this route. Steam itself is still needed for a Steam game copy.

**Install:** download `TTR-Meta-Link-1.0.0.zip`, install it using MO2's archive
installer, enable it below TTWVR, connect Link in the headset, and launch your
usual TTWVR game entry through MO2. Enabled=1 is the packaged default.

- Guarded correction for the reproduced Meta x86 IPC session-creation fault.
- Corrected OpenXR version request for native controller orientation.
- Native Meta eye FOV retained through camera rendering and submission.
- Changes are process-local; installed Meta/game DLLs and global runtime settings
  are not rewritten. An explicit different OpenXR runtime takes precedence.

Tested with Quest 3 / Oculus runtime 1.208.0 and the following exact files:

| File | SHA-256 |
| --- | --- |
| TTWVR.dll | `C9D047D4199C0458281D341AC9DC438A7C928999E19CDAD6F327F8A439A7F9EB` |
| RuntimeIPCServiceClient_32.dll | `D072611E9BA99EFC51FA5C9AAD9EECB6E1BA9FDDA8069EC96142241F75A57531` |

The standard `C:\Program Files\Oculus\Support\oculus-runtime` installation is
required. Unknown builds are refused; this beta has not been validated on other
runtime builds or headset models. The tested physical headset run confirmed
corrected hand orientation, distant-object fusion and head movement.

No DLSS, anti-aliasing, render-resolution or frame-generation preset is bundled.
No performance improvement is promised. No Meta or game binaries are redistributed.

If upgrading from the private TTWVR - Local Graphics mod, keep only one enabled
copy of LocalMetaLink.dll. Your personal graphics preset can remain separate.

[Filthy Casual TTR companion](https://github.com/nikamigaming-create/filthy-casual-ttr/releases/tag/v1.4.4)
adds the optional seated controls and inventory wheels. Both addons require the
original A Tale of Two Realities installation.
