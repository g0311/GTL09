#include "pch.h"

TMap<FString, FString> EditorINI;
const FString GDataDir = "Data";
const FString GCacheDir = "DerivedDataCache";

// Global client area size used by various modules
float CLIENTWIDTH = 1024.0f;
float CLIENTHEIGHT = 1024.0f;

// Global Engine instance (used in both _EDITOR and _GAME modes)
UEditorEngine GEngine;

UWorld* GWorld = nullptr;