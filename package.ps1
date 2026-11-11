$ErrorActionPreference='Stop'
$repo=$PSScriptRoot
$files=[ordered]@{
 'NVSE/Plugins/LocalMetaLink.dll'='build/LocalMetaLink.dll'
 'NVSE/Plugins/LocalMetaLink.ini'='LocalMetaLink.ini'
 'meta.ini'='meta.ini'
 'README.md'='README.md'
 'LICENSE'='LICENSE'
 'docs/assets/nikami-ttr-mods.png'='docs/assets/nikami-ttr-mods.png'
 'docs/assets/ART-PROVENANCE.md'='docs/assets/ART-PROVENANCE.md'
}
$commit=git -C $repo rev-parse HEAD
if($LASTEXITCODE){throw 'Source must be committed before packaging'}
$dirty=@(git -C $repo status --porcelain --untracked-files=normal)
$manifest=[ordered]@{addon='TTR Meta Link';version='1.1.0-beta';architecture='x86';sourceCommit="$commit".Trim();sourceDirty=($dirty.Count -gt 0);supportedTTWVR='E428C4BD1209D58D21F75108808C86049E26C76E58A164AD864A61B99696BDAB';supportedMetaIPC='D072611E9BA99EFC51FA5C9AAD9EECB6E1BA9FDDA8069EC96142241F75A57531';files=@($files.Keys|Sort-Object|ForEach-Object{@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $files[$_])).Hash}})}
Add-Type -AssemblyName System.IO.Compression
$out=Join-Path $repo 'outputs'
New-Item -ItemType Directory -Force $out|Out-Null
$destination=Join-Path $out 'TTR-Meta-Link-1.1.0-beta.zip'
$temp=Join-Path $out ('package-'+[guid]::NewGuid().ToString('N')+'.tmp')
$stream=[IO.File]::Open($temp,[IO.FileMode]::CreateNew)
$zip=[IO.Compression.ZipArchive]::new($stream,[IO.Compression.ZipArchiveMode]::Create,$true)
try{
 foreach($name in @(@($files.Keys)+'manifest.json'|Sort-Object)){
  $entry=$zip.CreateEntry($name,[IO.Compression.CompressionLevel]::Optimal)
  $entry.LastWriteTime=[DateTimeOffset]::new(2000,1,1,0,0,0,[TimeSpan]::Zero)
  $target=$entry.Open()
  try{
   if($name -eq 'manifest.json'){$bytes=[Text.Encoding]::UTF8.GetBytes(($manifest|ConvertTo-Json -Depth 5)+"`n");$target.Write($bytes,0,$bytes.Length)}
   else{$source=[IO.File]::OpenRead((Join-Path $repo $files[$name]));try{$source.CopyTo($target)}finally{$source.Dispose()}}
  }finally{$target.Dispose()}
 }
}finally{$zip.Dispose();$stream.Dispose()}
Move-Item -LiteralPath $temp -Destination $destination -Force
Write-Output $destination
