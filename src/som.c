#include "som.h"
#include <stdio.h>

bool ResolverArquivoAudio(const char *caminhoRelativo, char *bufferCaminho, size_t tamanhoBuffer){
    const char *diretorioAplicacao = GetApplicationDirectory();
    int caracteresEscritos =
        snprintf(bufferCaminho, tamanhoBuffer, "%s%s", diretorioAplicacao,
                 caminhoRelativo);
    if (caracteresEscritos >= 0 && (size_t)caracteresEscritos < tamanhoBuffer &&
        FileExists(bufferCaminho)) {
        return true;
    }

    caracteresEscritos = snprintf(bufferCaminho, tamanhoBuffer, "%s", caminhoRelativo);
    if (caracteresEscritos >= 0 && (size_t)caracteresEscritos < tamanhoBuffer &&
        FileExists(bufferCaminho)) {
        return true;
    }

    bufferCaminho[0] = '\0';
    TraceLog(LOG_ERROR, "Arquivo de audio nao encontrado: %s", caminhoRelativo);
    return false;
}

bool InicializarSons(Sons *estadoSons){
    char caminhoMusica[512];
    char caminhoPassos[512];

    if (!ResolverArquivoAudio("sons\\musica_fundo.mp3", caminhoMusica,sizeof(caminhoMusica)) ||
        !ResolverArquivoAudio("sons\\passos.mp3", caminhoPassos, sizeof(caminhoPassos))) {
        TraceLog(LOG_WARNING, "Audio incompleto; a gameplay continuara sem audio.");
        return false;
    }

    InitAudioDevice();

    if (!IsAudioDeviceReady()) {
        TraceLog(LOG_WARNING, "Audio indisponivel; a gameplay continuara sem audio.");
        return false;
    }

    estadoSons->dispositivoAudioAtivo = true;
    estadoSons->musicaFundo = LoadMusicStream(caminhoMusica);
    estadoSons->musicaPassos = LoadMusicStream(caminhoPassos);

    if (!IsMusicValid(estadoSons->musicaFundo) ||
        !IsMusicValid(estadoSons->musicaPassos)) {
        TraceLog(LOG_WARNING, "Falha ao carregar um ou mais audios; audio desativado.");
        return false;
    }

    estadoSons->musicaFundo.looping = true;
    estadoSons->musicaPassos.looping = true;
    return true;
}

void LiberarSons(Sons *estadoSons){
    if (!estadoSons->dispositivoAudioAtivo) return;

    if (IsMusicValid(estadoSons->musicaFundo)) {
        StopMusicStream(estadoSons->musicaFundo);
        UnloadMusicStream(estadoSons->musicaFundo);
    }

    if (IsMusicValid(estadoSons->musicaPassos)) {
        StopMusicStream(estadoSons->musicaPassos);
        UnloadMusicStream(estadoSons->musicaPassos);
    }

    CloseAudioDevice();
    estadoSons->dispositivoAudioAtivo = false;
}

void AtualizarPassos(Sons *estadoSons, bool teclasMovimentoPressionadas){
    if (!estadoSons->dispositivoAudioAtivo) return;

    if (teclasMovimentoPressionadas) {
        if (!estadoSons->somPassosTocando) {
            PlayMusicStream(estadoSons->musicaPassos);
            estadoSons->somPassosTocando = true;
        }

        UpdateMusicStream(estadoSons->musicaPassos);

    } else if (estadoSons->somPassosTocando) {
        StopMusicStream(estadoSons->musicaPassos);
        estadoSons->somPassosTocando = false;
    }
}
