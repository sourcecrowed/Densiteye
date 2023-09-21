#pragma once 
#include <filesystem>
#include <optional>
#include <string>

namespace Densiteye
{
  struct InputFilePathOption{};
  struct OutputFolderOption{};
  struct OutputNameOption{};
  struct DisableTransparentProcessingOption{};
  struct DisableOpaqueProcessingOption{};

  template<class... Args>
  struct ContainsArgument;

  template<class ArgType>
  struct ContainsArgument<ArgType>
  {
    static constexpr bool Value = false;
  };

  template<class ArgType, class ExistingType>
  struct ContainsArgument<ArgType, ExistingType>
  {
    static constexpr bool Value = std::is_same_v<ArgType, ExistingType>;
  };

  template<class ArgType, class ExistingType, class... ExistingTypes>
  struct ContainsArgument<ArgType, ExistingType, ExistingTypes...>
  {
    static constexpr bool Value = std::is_same_v<ArgType, ExistingType> || ContainsArgument<ArgType, ExistingTypes...>::Value;
  };

  template<class... Types>
  struct ContainsAllArgumentOptions
  {
    static constexpr bool Value = 
      ContainsArgument<InputFilePathOption>::Value &&
      ContainsArgument<OutputFolderOption>::Value &&
      ContainsArgument<OutputNameOption>::Value;
  };

  template<bool Cond, class A, class B>
  struct EnableIfTrueAElseB
  {
    using Type = B;
  };

  template<class A, class B>
  struct EnableIfTrueAElseB<true, A, B>
  {
    using Type = A;
  };

  template<class ArgumentsType, class... Types>
  class Arguments_
  {
    ArgumentsType &args; 

  public:
  
    inline Arguments_(ArgumentsType &args)
      : args(args)
    {
    }

    inline operator ArgumentsType() const
    {
      static_assert(ContainsArgument<InputFilePathOption, Types...>::Value, "Input file path is required");
      static_assert(ContainsArgument<OutputFolderOption, Types...>::Value, "Output folder is required");
      static_assert(ContainsArgument<OutputNameOption, Types...>::Value, "Output name is required");

      // DisableTransparentProcessingOption is optional
      // DisableOpaqueProcessingOption is optional

      return std::move(args);
    }

    inline auto InputFilePath(std::filesystem::path const &path) &&
    {
      static_assert(!ContainsArgument<InputFilePathOption, Types...>::Value, "Only one input file is allowed");
      args.InputFilePath = path;
      return Arguments_<ArgumentsType, Types..., InputFilePathOption>(args);
    }

    inline auto OutputFolder(std::filesystem::path const &path) &&
    {
      static_assert(!ContainsArgument<OutputFolderOption, Types...>::Value, "Only one output folder is allowed");
      args.OutputFolder = path;
      return Arguments_<ArgumentsType, Types..., OutputFolderOption>(args);
    }

    inline auto OutputName(std::string_view const &name) &&
    {
      static_assert(!ContainsArgument<OutputNameOption, Types...>::Value, "Only one output folder is allowed");
      args.OutputName = name;
      return Arguments_<ArgumentsType, Types..., OutputNameOption>(args);
    }

    inline auto DisableTransparentProcessing()
    {
      static_assert(!ContainsArgument<DisableTransparentProcessingOption, Types...>::Value, "Disabling of transparent processing has already been set");
      args.DisableTransparentProcessing = true;
      return Arguments_<ArgumentsType, Types..., DisableTransparentProcessingOption>(args);
    }

    inline auto DisableOpaqueProcessing()
    {
      static_assert(!ContainsArgument<DisableOpaqueProcessingOption, Types...>::Value, "Disabling of opaque processing has already been set");
      args.DisableOpaqueProcessing = true;
      return Arguments_<ArgumentsType, Types..., DisableOpaqueProcessingOption>(args);
    }
  };

  class Arguments
  {
    template<class ArgumentsType, class... Types>
    friend class Arguments_;

    std::filesystem::path InputFilePath;
    std::filesystem::path OutputFolder;
    std::string OutputName;
    bool DisableTransparentProcessing = false;
    bool DisableOpaqueProcessing = false;

    inline Arguments(){ }

  public:

    auto const &GetInputFilePath() const { return InputFilePath; }
    auto const &GetOutputFolder() const { return OutputFolder; }
    auto const &GetOutputName() const { return OutputName; }
    bool IsTransparentProcessingDisabled() const { return DisableTransparentProcessing; }
    bool IsOpaqueProcessingDisabled() const { return DisableOpaqueProcessing; }

    static inline auto Build()
    {
      Arguments args;
      return Arguments_<Arguments>(args);
    }
  };
}
