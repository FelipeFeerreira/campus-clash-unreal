#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CalculusWidgets.generated.h"

class ACalculusClassroom;
class UCalculusWidget;
class UTextBlock;
class UBorder;
class UButton;
class UVerticalBox;
class UHorizontalBox;

/** Opção clicável (dificuldade, resposta ou passo do quadro). Também responde às teclas 1-9 via o painel. */
UCLASS()
class SCHOOL_API UCalcChoice : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UCalculusWidget> Panel;
    int32 Index = 0;
    FText Caption;
    /** Já escolhida (passos do quadro): fica destacada e não aceita clique. */
    bool bPicked = false;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
private:
    UPROPERTY() TObjectPtr<UBorder> Border;
    void Paint(bool bHover);
};

/** Interface da aula de Cálculo: fala do professor, dificuldade, perguntas, quadro e resultado. */
UCLASS()
class SCHOOL_API UCalculusWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<ACalculusClassroom> Room;
    /** Clique/tecla numa opção da etapa atual. */
    void Choose(int32 Index);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    UPROPERTY() TObjectPtr<UBorder> DialogBox;
    UPROPERTY() TObjectPtr<UTextBlock> DialogName;
    UPROPERTY() TObjectPtr<UTextBlock> DialogText;
    UPROPERTY() TObjectPtr<UBorder> MainPanel;
    UPROPERTY() TObjectPtr<UTextBlock> Title;
    UPROPERTY() TObjectPtr<UTextBlock> Info;
    UPROPERTY() TObjectPtr<UTextBlock> Body;
    UPROPERTY() TObjectPtr<UVerticalBox> Choices;
    UPROPERTY() TObjectPtr<UTextBlock> FeedbackText;
    UPROPERTY() TObjectPtr<UButton> RetryButton;
    UPROPERTY() TObjectPtr<UButton> CampusButton;
    UPROPERTY() TObjectPtr<UButton> ClearButton;
    /** Passos do quadro já clicados (índices exibidos), na ordem escolhida. */
    TArray<int32> BoardPicked;
    /** Muda quando as opções precisam ser recriadas. */
    FString ChoicesSignature;
    /** Momento da última troca de tela; entradas logo depois são ignoradas (evita clique/tecla "vazar" para a tela nova). */
    double LastScreenChange = -1000.0;
    void MarkScreenChange();
    bool IsInputReady() const;
    void Refresh();
    void RebuildChoices(const TArray<FText>& Captions);
    UFUNCTION() void HandleLessonChanged();
    UFUNCTION() void OnRetry();
    UFUNCTION() void OnCampus();
    UFUNCTION() void OnClear();
};
