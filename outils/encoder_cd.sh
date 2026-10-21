# usage : encoder_cd.sh <dossier_wav> <cover.jpg> <dossier_sortie> <artiste> <album> <année> <titres.txt> [total_pistes_album] [premiere_piste]
# WAV du CD -> MP3 320 kbit/s + tags + pochette baseline intégrée
wav="$1"; cover="$2"; sortie="$3"; artiste="$4"; album="$5"; annee="$6"
mapfile -t titres < "$7"; totalalbum="${8:-${#titres[@]}}"; premiere="${9:-1}"
mkdir -p "$sortie"; total=${#titres[@]}
for n in $(seq $premiere $((premiere + total - 1))); do
  nn=$(printf "%02d" $n); t="${titres[$((n-premiere))]}"; nom=$(echo "$t" | tr '\\/:*?"<>|' '_________')
  ffmpeg -v error -y -i "$wav/$nn.wav" -i "$cover" -map 0:a -map 1:v -c:a libmp3lame -b:a 320k -c:v copy \
    -id3v2_version 3 -metadata title="$t" -metadata artist="$artiste" -metadata album_artist="$artiste" \
    -metadata album="$album" -metadata date="$annee" -metadata track="$n/$totalalbum" \
    -metadata:s:v title="Album cover" -metadata:s:v comment="Cover (front)" \
    "$sortie/$nn $nom.mp3" && echo "ok $nn $t"
done
