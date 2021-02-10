#
# The list of VS 2019 components: https://docs.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-community?vs-2019&view=vs-2019
#

Function InstallVS
{
  Param
  (
    [String]$WorkLoads,
    [String]$Sku,
    [String] $VSBootstrapperURL
  )

  $exitCode = -1

  try
  {
    Write-Host "Downloading Bootstrapper ..."
    Invoke-WebRequest -Uri $VSBootstrapperURL -OutFile "${env:Temp}\vs_$Sku.exe"

    $FilePath = "${env:Temp}\vs_$Sku.exe"
    $Arguments = ($WorkLoads, '--quiet', '--norestart', '--wait', '--nocache')

    Write-Host "Starting Install ..."
    $process = Start-Process -FilePath $FilePath -ArgumentList $Arguments -Wait -PassThru
    $exitCode = $process.ExitCode

    if ($exitCode -eq 0 -or $exitCode -eq 3010)
    {
      Write-Host -Object 'Installation successful'
      return $exitCode
    }
    else
    {
      Write-Host -Object "Non zero exit code returned by the installation process : $exitCode."

      # this wont work because of log size limitation in extension manager
      # Get-Content $customLogFilePath | Write-Host

      exit $exitCode
    }
  }
  catch
  {
    Write-Host -Object "Failed to install Visual Studio. Check the logs for details in $customLogFilePath"
    Write-Host -Object $_.Exception.Message
    exit -1
  }
}

Write-Host "Step 1. Setting parameters..." -ForegroundColor Cyan

$WorkLoads = '--add Microsoft.VisualStudio.Product.Community ' + `
'--add Microsoft.Net.Component.4.7.2.SDK ' + `
'--add Microsoft.VisualStudio.Component.VC.Redist.MSM ' + `
'--add Microsoft.VisualCpp.CRT.Redist.MSM ' + `
'--add Component.Linux.CMake ' + `
'--add Microsoft.VisualStudio.VC.Ide.Linux.CMake ' + `
'--add Component.MDD.Linux ' + `
'--add Microsoft.VisualStudio.VC.Ide.Linux ' + `
'--add Microsoft.VisualStudio.VC.Ide.Linux.Resources ' + `
'--add Microsoft.VisualStudio.Workload.NativeDesktop ' + `
'--add Microsoft.VisualCpp.IFC.X86 ' + `
'--add Microsoft.VisualCpp.IFC.X64 ' + `
'--add Microsoft.VisualStudio.Component.VC.ATL ' + `
'--add Microsoft.VisualStudio.VC.Ide.ATL ' + `
'--add Microsoft.VisualStudio.VC.Ide.ATL.Resources ' + `
'--add Microsoft.VisualCpp.ATL.X86 ' + `
'--add Microsoft.VisualCpp.ATL.X64 ' + `
'--add Microsoft.VisualCpp.ATL.Source ' + `
'--add Microsoft.VisualCpp.ATL.Headers ' + `
'--add Microsoft.VisualStudio.Component.VC.CMake.Project ' + `
'--add Microsoft.VisualStudio.ComponentGroup.WebToolsExtensions.CMake ' + `
'--add Microsoft.VisualStudio.VC.CMake ' + `
'--add Microsoft.VisualStudio.VC.CMake.Project ' + `
'--add Microsoft.VisualStudio.VC.ExternalBuildFramework ' + `
'--add Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Core ' + `
'--add Microsoft.VisualStudio.Component.VC.Redist.14.Latest ' + `
'--add Microsoft.VisualStudio.VC.Templates.UnitTest ' + `
'--add Microsoft.VisualStudio.VC.UnitTest.Desktop.Build.Core ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.V1.CPP ' + `
'--add Microsoft.VisualStudio.VC.Templates.UnitTest.Resources ' + `
'--add Microsoft.VisualStudio.VC.Templates.Desktop ' + `
'--add Microsoft.VisualStudio.Workload.ManagedDesktop ' + `
'--add Microsoft.VisualStudio.Debugger.ImmersiveActivateHelper.Msi ' + `
'--add Microsoft.VisualStudio.Debugger.JustInTime ' + `
'--add Microsoft.VisualStudio.Debugger.JustInTime.Msi ' + `
'--add Microsoft.VisualStudio.Blend.Resources ' + `
'--add Microsoft.VisualStudio.Blend ' + `
'--add Microsoft.VisualStudio.Component.ManagedDesktop.Prerequisites ' + `
'--add Microsoft.VisualStudio.Templates.VB.Wpf ' + `
'--add Microsoft.VisualStudio.Templates.VB.Wpf.Resources ' + `
'--add Microsoft.VisualStudio.Templates.VB.Winforms ' + `
'--add Microsoft.VisualStudio.Templates.CS.Wpf ' + `
'--add Microsoft.VisualStudio.Templates.CS.Wpf.Resources ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX64.TargetX64.Resources ' + `
'--add Microsoft.VisualCpp.PGO.X86 ' + `
'--add Microsoft.VisualStudio.VC.Ide.Linux.CMake.Resources ' + `
'--add Microsoft.VisualCpp.PGO.X64 ' + `
'--add Microsoft.VisualCpp.PGO.Headers ' + `
'--add Microsoft.VisualCpp.CRT.x86.Store ' + `
'--add Microsoft.VisualCpp.CRT.x86.OneCore.Desktop ' + `
'--add Microsoft.VisualCpp.CRT.Redist.x86.OneCore.Desktop ' + `
'--add Microsoft.VisualCpp.CRT.Redist.x64.OneCore.Desktop ' + `
'--add Microsoft.VisualStudio.PackageGroup.VC.Tools.x86 ' + `
'--add Microsoft.VisualCpp.Tools.HostX86.TargetX64 ' + `
'--add Microsoft.VisualCpp.Tools.Hostx86.Targetx64.Resources ' + `
'--add Microsoft.VisualCpp.Tools.HostX86.TargetX86 ' + `
'--add Microsoft.VisualCpp.Tools.HostX86.TargetX86.Resources ' + `
'--add Microsoft.VisualCpp.Tools.Core.Resources ' + `
'--add Microsoft.VisualCpp.Tools.Core.x86 ' + `
'--add Microsoft.VisualCpp.DIA.SDK ' + `
'--add Microsoft.VisualCpp.CRT.x86.Desktop ' + `
'--add Microsoft.VisualCpp.CRT.x64.Desktop ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.Native ' + `
'--add Microsoft.VisualCpp.CRT.Source ' + `
'--add Microsoft.VisualCpp.CRT.Redist.X86 ' + `
'--add Microsoft.VisualCpp.CRT.Redist.X64 ' + `
'--add Microsoft.VisualCpp.CRT.Redist.Resources ' + `
'--add Microsoft.VisualCpp.RuntimeDebug.14 ' + `
'--add Microsoft.VisualCpp.RuntimeDebug.14 ' + `
'--add Microsoft.VisualStudio.Component.Windows10SDK.17763 ' + `
'--add Win10SDK_10.0.17763 ' + `
'--add Microsoft.VisualStudio.VC.Templates.Pro.Resources ' + `
'--add Microsoft.VisualStudio.Component.Graphics.Tools ' + `
'--add Microsoft.VisualStudio.VC.Items.Pro ' + `
'--add Microsoft.VisualStudio.VC.Ide.x64 ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.X64.v142 ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.X64 ' + `
'--add Microsoft.VS.VC.MSBuild.X64.Resources ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.ARM.v142 ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.ARM ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.x86.v142 ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.X86 ' + `
'--add Microsoft.VisualStudio.Templates.CS.Winforms ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.Base ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.Base.Resources ' + `
'--add Microsoft.VisualStudio.Component.VC.DiagnosticTools ' + `
'--add Microsoft.VisualStudio.VC.Ide.Dskx ' + `
'--add Microsoft.VisualStudio.VC.Ide.Dskx.Resources ' + `
'--add Microsoft.VisualStudio.VC.Ide.Base ' + `
'--add Microsoft.VisualStudio.VC.Ide.LanguageService ' + `
'--add Microsoft.VisualStudio.VC.Llvm.Base ' + `
'--add Microsoft.VisualStudio.VC.Ide.Core ' + `
'--add Microsoft.VisualStudio.VC.Ide.ProjectSystem ' + `
'--add Microsoft.VisualStudio.VC.Ide.ProjectSystem.Resources ' + `
'--add Microsoft.VisualStudio.VC.Ide.Core.VCProjectEngine ' + `
'--add Microsoft.VisualStudio.VC.Ide.LanguageService.Resources ' + `
'--add Microsoft.VisualStudio.VC.Ide.Base.Resources ' + `
'--add Microsoft.VisualStudio.Component.DiagnosticTools ' + `
'--add Microsoft.DiagnosticsHub.DotNetAsync.ExternalDependencies ' + `
'--add Microsoft.DiagnosticsHub.DotNetAsync ' + `
'--add Microsoft.DiagnosticsHub.DotNetAsync.Targeted ' + `
'--add Microsoft.DiagnosticsHub.EventsViewerTool.ExternalDependencies ' + `
'--add Microsoft.DiagnosticsHub.EventsViewerTool ' + `
'--add Microsoft.DiagnosticsHub.EventsViewerTool.Targeted ' + `
'--add Microsoft.Icecap.Analysis ' + `
'--add Microsoft.Icecap.Analysis.Targeted ' + `
'--add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ' + `
'--add Microsoft.Icecap.Analysis.Resources ' + `
'--add Microsoft.Icecap.Analysis.Resources.Targeted ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.Extensions ' + `
'--add Microsoft.Icecap.Collection.Msi ' + `
'--add Microsoft.Icecap.Collection.Msi.Targeted ' + `
'--add Microsoft.Icecap.Collection.Msi.Resources ' + `
'--add Microsoft.Icecap.Collection.Msi.Resources.Targeted ' + `
'--add Microsoft.DiagnosticsHub.DatabaseTool.ExternalDependencies ' + `
'--add Microsoft.DiagnosticsHub.DatabaseTool ' + `
'--add Microsoft.DiagnosticsHub.DatabaseTool.Targeted ' + `
'--add Microsoft.DiagnosticsHub.DotNetObjectAlloc.ExternalDependencies ' + `
'--add Microsoft.DiagnosticsHub.DotNetObjectAlloc ' + `
'--add Microsoft.DiagnosticsHub.DotNetObjectAlloc.Targeted ' + `
'--add Microsoft.DiagnosticsHub.Instrumentation ' + `
'--add Microsoft.DiagnosticsHub.CpuSampling.ExternalDependencies ' + `
'--add Microsoft.DiagnosticsHub.CpuSampling ' + `
'--add Microsoft.DiagnosticsHub.CpuSampling.Targeted ' + `
'--add Microsoft.PackageGroup.DiagnosticsHub.Platform ' + `
'--add Microsoft.DiagnosticsHub.Runtime.ExternalDependencies ' + `
'--add SQLiteCore ' + `
'--add Microsoft.VisualStudio.Graphics.EnableTools ' + `
'--add SQLiteCore.Targeted ' + `
'--add Microsoft.DiagnosticsHub.Runtime.ExternalDependencies.Targeted ' + `
'--add Microsoft.DiagnosticsHub.Runtime ' + `
'--add Microsoft.VisualStudio.Graphics.Analyzer.Targeted ' + `
'--add Microsoft.DiagnosticsHub.Runtime.Targeted ' + `
'--add Microsoft.DiagnosticsHub.Collection.ExternalDependencies.x64 ' + `
'--add Microsoft.DiagnosticsHub.Collection ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.TestPlatform.Legacy ' + `
'--add Microsoft.VisualStudio.TestTools.TP.Legacy.Tips.Common ' + `
'--add Microsoft.VisualStudio.Component.VC.CoreIde ' + `
'--add Microsoft.VisualStudio.VC.Ide.Pro ' + `
'--add Microsoft.Net.Core.Component.SDK.2.1 ' + `
'--add Microsoft.NetCore.Templates.2.1.3.1.300-servicing-015161 ' + `
'--add Microsoft.NetCore.SharedFramework.2.1.2.1.19 ' + `
'--add Microsoft.AspNetCore.SharedFramework.2.1.2.1.19 ' + `
'--add Microsoft.VisualStudio.PackageGroup.VC.CoreIDE.Reduced ' + `
'--add Microsoft.VisualStudio.Component.IntelliCode ' + `
'--add Microsoft.VisualStudio.PackageGroup.VC.CoreIDE.Express ' + `
'--add Microsoft.VisualStudio.VsDevCmd.Ext.IntelliCode ' + `
'--add Microsoft.VisualStudio.IntelliCode ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.Templates.Managed ' + `
'--add Microsoft.VisualStudio.Templates.VB.MSTestv2.Desktop.UnitTest ' + `
'--add Microsoft.VisualStudio.Templates.CS.MSTestv2.Desktop.UnitTest ' + `
'--add Microsoft.VisualStudio.Templates.VB.ManagedCore ' + `
'--add Microsoft.VisualStudio.Templates.VB.Shared.Resources ' + `
'--add Microsoft.VisualStudio.Templates.VB.ManagedCore.Resources ' + `
'--add Microsoft.VisualStudio.Templates.CS.ManagedCore ' + `
'--add Microsoft.VisualStudio.VC.Ide.WinXPlus ' + `
'--add Microsoft.VisualStudio.Templates.AssemblyInfo.Wizard ' + `
'--add Microsoft.VisualStudio.Templates.Editorconfig.Wizard.Setup ' + `
'--add Templates.Editorconfig.SolutionFile.Setup ' + `
'--add Microsoft.VisualStudio.Templates.CS.Shared.Resources ' + `
'--add Microsoft.VisualStudio.Templates.CS.ManagedCore.Resources ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.Extensions.X86 ' + `
'--add Microsoft.VisualStudio.VC.Ide.VCPkgDatabase ' + `
'--add Microsoft.VisualStudio.Component.ManagedDesktop.Core ' + `
'--add Microsoft.VisualStudio.VC.Ide.ResourceEditor ' + `
'--add Microsoft.VisualStudio.VC.Ide.ResourceEditor.Resources ' + `
'--add Microsoft.VisualStudio.VC.Ide.Core.VCProjectEngine.Resources ' + `
'--add Microsoft.VisualStudio.PackageGroup.ConnectedServices ' + `
'--add Microsoft.VisualStudio.AppCapDesigner ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX86.TargetX86.Resources ' + `
'--add Microsoft.VisualStudio.Graphics.Appid ' + `
'--add Microsoft.VisualStudio.Graphics.Appid.Resources ' + `
'--add Microsoft.VisualStudio.Component.Debugger.JustInTime ' + `
'--add Microsoft.VisualCpp.Redist.14.Latest ' + `
'--add Microsoft.VisualCpp.CRT.Headers ' + `
'--add Microsoft.VisualStudio.Graphics.Viewers ' + `
'--add Microsoft.ComponentGroup.Blend ' + `
'--add Microsoft.VisualStudio.Blend.Msi ' + `
'--add Microsoft.VisualStudio.VC.Ide.Pro.Resources ' + `
'--add Microsoft.VisualStudio.VC.Templates.Pro ' + `
'--add Microsoft.VisualStudio.Graphics.Msi ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.Extensions.X86.Resources ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.ConcurrencyCheck.X86 ' + `
'--add Microsoft.VisualStudio.ConnectedServices.Azure.Storage ' + `
'--add Microsoft.VisualStudio.ConnectedServices.Azure.KeyVault ' + `
'--add Microsoft.VisualStudio.ConnectedServices.Azure.Authentication ' + `
'--add Microsoft.VisualStudio.ConnectedServices.Core ' + `
'--add Microsoft.VisualStudio.XamlDiagnostics ' + `
'--add Microsoft.VisualStudio.XamlDiagnostics.Resources ' + `
'--add Microsoft.VisualStudio.ConnectedServices.Wcf ' + `
'--add Microsoft.PackageGroup.Icecap.Core ' + `
'--add Microsoft.VisualStudio.XamlDesigner ' + `
'--add Microsoft.VisualStudio.XamlDesigner.Resources ' + `
'--add Microsoft.VisualStudio.XamlDesigner.Executables ' + `
'--add Microsoft.VisualStudio.XamlShared ' + `
'--add Microsoft.VisualStudio.XamlShared.Resources ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.Managed ' + `
'--add Microsoft.IntelliTrace.Core ' + `
'--add Microsoft.IntelliTrace.Core.Concord ' + `
'--add Microsoft.IntelliTrace.Core.Targeted ' + `
'--add Microsoft.VisualStudio.TestTools.TestGeneration ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.Enterprise ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.V2.CLI ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.ConcurrencyCheck.X86.Resources ' + `
'--add Microsoft.VisualStudio.TestTools.MSTestV2.WizardExtension.UnitTest ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.Core ' + `
'--add Microsoft.VisualStudio.TestWindow.SourceBasedTestDiscovery ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.V1.CLI ' + `
'--add Microsoft.DiagnosticsHub.Collection.Service ' + `
'--add Microsoft.DiagnosticsHub.Collection.StopService.Install ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX64.TargetX86.Resources ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX64.TargetX64 ' + `
'--add Microsoft.Net.4.TargetingPack ' + `
'--add Microsoft.VisualStudio.TestTools.Templates.Managed.Resources ' + `
'--add Microsoft.VisualStudio.TestTools.TP.Legacy.Common.Res ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.Legacy.Core.Resources ' + `
'--add Microsoft.VisualStudio.Component.Common.Azure.Tools ' + `
'--add Microsoft.VisualStudio.Web.Azure.Common ' + `
'--add Microsoft.VisualStudio.Azure.CommonAzureTools ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.Legacy.Tips ' + `
'--add Microsoft.VisualStudio.TestTools.Templates.Managed ' + `
'--add Microsoft.VisualStudio.TextTemplating.MSBuild ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.Legacy.Tips.Resources ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.Legacy.Common ' + `
'--add Microsoft.VisualStudio.TextTemplating.Integration ' + `
'--add Microsoft.VisualStudio.Templates.VB.Shared ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.Legacy.Core ' + `
'--add Microsoft.VisualStudio.TextTemplating.Core ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.Extensions.X64 ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.Extensions.X64.Resources ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.Legacy.Agent ' + `
'--add Microsoft.VisualStudio.Templates.CS.Shared ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.TestPlatform.IDE ' + `
'--add Microsoft.VisualStudio.TestTools.TestWIExtension ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.IDE ' + `
'--add Microsoft.VisualStudio.TextTemplating.Integration.Resources ' + `
'--add Microsoft.VisualStudio.IISExpress.Msi ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.ConcurrencyCheck.X64 ' + `
'--add Microsoft.VisualCpp.CRT.ClickOnce.Msi ' + `
'--add Microsoft.SQL.ClickOnceBootstrapper.Msi ' + `
'--add Microsoft.Net.ClickOnceBootstrapper ' + `
'--add Microsoft.VisualStudio.ClickOnce.Resources ' + `
'--add Microsoft.VisualStudio.ConnectedServices.Office365 ' + `
'--add Microsoft.VisualStudio.ClickOnce ' + `
'--add Microsoft.VisualStudio.ProjectSystem.Managed.CommonFiles ' + `
'--add Microsoft.VisualStudio.ProjectSystem.Managed ' + `
'--add Microsoft.Component.MSBuild ' + `
'--add Microsoft.NuGet.Build.Tasks.Setup ' + `
'--add Microsoft.VisualStudio.JavaScript.EdgeAdapterHost ' + `
'--add Microsoft.VisualStudio.JavaScript.EdgeAdapter ' + `
'--add Microsoft.VisualStudio.PackageGroup.JavaScript.ChromeAdapterHost ' + `
'--add Microsoft.VisualStudio.JavaScript.ChromeAdapterHost ' + `
'--add Microsoft.VisualStudio.JavaScript.ChromeAdapter.v2 ' + `
'--add Microsoft.VisualStudio.Debugger.Script ' + `
'--add Microsoft.VisualStudio.Debugger.Script ' + `
'--add Microsoft.VisualStudio.Package.NodeJs ' + `
'--add Microsoft.VisualStudio.PackageGroup.IntelliTrace.Core ' + `
'--add Microsoft.Net.Component.4.8.SDK ' + `
'--add Microsoft.Net.4.8.SDK ' + `
'--add Microsoft.VisualStudio.Component.DockerTools ' + `
'--add Microsoft.IntelliTrace.ProfilerProxy.Msi.x64 ' + `
'--add Microsoft.IntelliTrace.ProfilerProxy.Msi ' + `
'--add Microsoft.VisualStudio.JavaScript.ChromeAdapter ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.ConcurrencyCheck.X64.Resources ' + `
'--add Microsoft.VisualStudio.Web.PerformanceTools ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.CodeCoverage ' + `
'--add Microsoft.SmartDevice.Connectivity ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.MSTestV2.Managed ' + `
'--add Microsoft.VisualStudio.Web.PerformanceTools.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.Script.Msi ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.TestPlatform.V2.CLI ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.TestPlatform.V1.CLI ' + `
'--add Microsoft.VisualStudio.Debugger.Script.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.Script.Resources ' + `
'--add Microsoft.VisualStudio.PackageGroup.MinShell.Interop ' + `
'--add Microsoft.VisualStudio.TestTools.TP.Legacy.Tips.Msi ' + `
'--add Microsoft.Net.ComponentGroup.DevelopmentPrerequisites ' + `
'--add Microsoft.Net.Component.4.7.2.TargetingPack ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.Legacy.TestTools ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.Legacy.Professional ' + `
'--add Microsoft.Net.Cumulative.TargetingPack.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.OneCore.x64 ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.OneCore.x64 ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.OneCore.x64.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.OneCore.ManagedSupport ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.DataCollectors ' + `
'--add Microsoft.VisualStudio.Debugger.OneCore.x64.Resources ' + `
'--add Microsoft.VisualStudio.Component.SQL.CLR ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.OneCore.ManagedSupport ' + `
'--add Microsoft.VisualCpp.CRT.x64.Store ' + `
'--add Microsoft.VisualStudio.ComponentGroup.WebToolsExtensions ' + `
'--add Microsoft.VisualStudio.ProTools ' + `
'--add Microsoft.VisualStudio.Component.IISExpress ' + `
'--add sqlsysclrtypes ' + `
'--add Microsoft.VisualStudio.Component.VC.Modules.x86.x64 ' + `
'--add sqlsysclrtypes ' + `
'--add SQLCommon ' + `
'--add Microsoft.VisualStudio.ProTools.Resources ' + `
'--add Microsoft.VisualCpp.CRT.x64.OneCore.Desktop ' + `
'--add Microsoft.VisualStudio.PackageGroup.ClickOnce ' + `
'--add Microsoft.VisualStudio.Web.Scaffolding ' + `
'--add Microsoft.VisualStudio.Component.TextTemplating ' + `
'--add Microsoft.VisualStudio.WebTools.WSP.FSA.Resources ' + `
'--add Microsoft.VisualStudio.Containers.Tools.Extensions ' + `
'--add Microsoft.VisualStudio.Package.DockerTools ' + `
'--add Microsoft.VisualStudio.VC.Ide.MDD ' + `
'--add Microsoft.VisualStudio.VC.Ide.Linux.ConnectionManager ' + `
'--add Microsoft.VisualStudio.PackageGroup.ClickOnce.MSBuild ' + `
'--add Microsoft.VisualStudio.VisualC.Utilities.Resources ' + `
'--add Microsoft.ClickOnce.SignTool.Msi ' + `
'--add Microsoft.VisualStudio.WebToolsExtensions ' + `
'--add Microsoft.VisualStudio.WebTools ' + `
'--add Microsoft.ClickOnce.BootStrapper.Msi.Resources ' + `
'--add Microsoft.ClickOnce.BootStrapper.Msi ' + `
'--add Microsoft.VisualStudio.WebTools.Resources ' + `
'--add Microsoft.VisualStudio.WebTools.WSP.FSA ' + `
'--add Microsoft.VisualStudio.Windows.Forms ' + `
'--add Microsoft.VisualStudio.Package.ContainersTools ' + `
'--add Microsoft.VisualCpp.Tools.HostX64.TargetX86 ' + `
'--add Microsoft.VisualStudio.Component.JavaScript.Diagnostics ' + `
'--add Microsoft.VisualStudio.PackageGroup.JavaScript.EdgeAdapterHost ' + `
'--add Microsoft.VisualStudio.Package.DockerTools.BuildTools ' + `
'--add Microsoft.VisualStudio.TemplateEngine ' + `
'--add Microsoft.NetStandard.VB.ProjectTemplates ' + `
'--add Microsoft.NetStandard.CSharp.ProjectTemplates ' + `
'--add Microsoft.NetCore.CSharp.ProjectTemplates.Desktop ' + `
'--add Microsoft.WebTools.Shared ' + `
'--add Microsoft.VisualStudio.VisualC.Utilities ' + `
'--add Microsoft.VisualStudio.PackageGroup.Debugger.Script ' + `
'--add Microsoft.VisualStudio.VC.Ide.Linux.ConnectionManager.Resources ' + `
'--add Microsoft.VisualStudio.Component.Roslyn.LanguageServices ' + `
'--add Microsoft.VisualCpp.Redist.14.Latest ' + `
'--add Microsoft.VisualCpp.Redist.14 ' + `
'--add Microsoft.VisualStudio.Graphics.Viewers.Resources ' + `
'--add Microsoft.WebTools.DotNet.Core.ItemTemplates ' + `
'--add Microsoft.VisualStudio.StaticAnalysis ' + `
'--add Microsoft.VisualStudio.StaticAnalysis.Resources ' + `
'--add Microsoft.CodeAnalysis.Compilers ' + `
'--add Microsoft.VisualStudio.InteractiveWindow ' + `
'--add Microsoft.Net.4.7.2.TargetingPack ' + `
'--add Microsoft.Net.4.7.2.TargetingPack.Resources ' + `
'--add Microsoft.DiaSymReader.Native ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX64.TargetX86 ' + `
'--add Microsoft.Net.4.7.2.SDK ' + `
'--add Microsoft.VisualCpp.Redist.14 ' + `
'--add Roslyn.VisualStudio.Setup.ServiceHub ' + `
'--add Microsoft.VisualStudio.Debugger.CoreClr.x64 ' + `
'--add Microsoft.VisualStudio.Community.Extra.Resources ' + `
'--add Microsoft.VisualStudio.PackageGroup.Core ' + `
'--add Microsoft.VisualStudio.CodeSense.Community ' + `
'--add Microsoft.VisualStudio.TestTools.TeamFoundationClient ' + `
'--add Microsoft.VisualStudio.PackageGroup.Debugger.Core ' + `
'--add Microsoft.VisualStudio.Community.Extra ' + `
'--add Microsoft.VisualStudio.PackageGroup.Debugger.TimeTravel.Record ' + `
'--add Microsoft.VisualStudio.Debugger.TimeTravel.Runtime ' + `
'--add Microsoft.VisualStudio.Debugger.TimeTravel.Runtime ' + `
'--add Microsoft.VisualStudio.VC.Ide.Debugger ' + `
'--add Microsoft.VisualStudio.VC.Ide.Debugger.Concord ' + `
'--add Microsoft.VisualStudio.VC.Ide.Debugger.Concord.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.TimeTravel.Agent ' + `
'--add Microsoft.VisualStudio.WebTools.MSBuild ' + `
'--add Microsoft.VisualStudio.Debugger.TimeTravel.Record ' + `
'--add Microsoft.VisualStudio.Debugger.VSCodeDebuggerHost ' + `
'--add Microsoft.VisualStudio.VC.Ide.Common.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.Parallel ' + `
'--add Microsoft.VisualCpp.Tools.HostX64.TargetX86.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.Parallel.Resources ' + `
'--add Microsoft.VisualCpp.Tools.HostX64.TargetX64 ' + `
'--add Microsoft.VisualCpp.Tools.HostX64.TargetX64.Resources ' + `
'--add Microsoft.VisualStudio.VC.Ide.Debugger.Resources ' + `
'--add Microsoft.VisualStudio.VC.Ide.Common ' + `
'--add Microsoft.VisualStudio.Debugger.CollectionAgents ' + `
'--add Microsoft.VisualStudio.Debugger.Managed ' + `
'--add Microsoft.CodeAnalysis.VisualStudio.Setup ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX86.TargetX64 ' + `
'--add Microsoft.VisualStudio.VsDevCmd.Ext.NetFxSdk ' + `
'--add Microsoft.VisualCpp.Premium.Tools.Hostx86.Targetx64.Resources ' + `
'--add Microsoft.CodeAnalysis.ExpressionEvaluator ' + `
'--add Microsoft.VisualStudio.TestTools.Pex.Common ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.Managed.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.Managed.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.TargetComposition ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.Managed ' + `
'--add Microsoft.VisualStudio.Debugger.TargetComposition ' + `
'--add Microsoft.VisualStudio.Debugger.Remote ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.Remote ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX86.TargetX86 ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.Remote.Resources ' + `
'--add Microsoft.VisualStudio.Component.Roslyn.Compiler ' + `
'--add Microsoft.VisualStudio.Component.NuGet ' + `
'--add Microsoft.VisualStudio.Debugger.Remote ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.Remote ' + `
'--add Microsoft.CredentialProvider ' + `
'--add Microsoft.VisualStudio.NuGet.Licenses ' + `
'--add Microsoft.VisualStudio.PackageGroup.Community ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.Remote.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.Remote.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.Remote.Resources ' + `
'--add Microsoft.VisualStudio.Debugger.Package.DiagHub.Client.VSx86 ' + `
'--add Microsoft.VisualStudio.Debugger.Remote.DiagHub.Client ' + `
'--add Microsoft.VisualStudio.Debugger ' + `
'--add Microsoft.DiaSymReader.PortablePdb ' + `
'--add Microsoft.VisualStudio.Debugger.Remote.DiagHub.Client ' + `
'--add Microsoft.VisualStudio.VC.MSVCDis ' + `
'--add Microsoft.VisualStudio.ScriptedHost ' + `
'--add Microsoft.VisualStudio.ScriptedHost.Targeted ' + `
'--add Microsoft.VisualStudio.ScriptedHost.Resources ' + `
'--add Microsoft.IntelliTrace.DiagnosticsHub ' + `
'--add Microsoft.VisualStudio.Debugger.Concord ' + `
'--add Microsoft.VisualStudio.Debugger.Concord.Resources ' + `
'--add Microsoft.VisualStudio.AppResponsiveness ' + `
'--add Microsoft.VisualStudio.AppResponsiveness.Targeted ' + `
'--add Microsoft.VisualStudio.Debugger.Resources ' + `
'--add Microsoft.VisualStudio.AppResponsiveness.Resources ' + `
'--add Microsoft.VisualStudio.ClientDiagnostics ' + `
'--add Microsoft.VisualStudio.ClientDiagnostics.Targeted ' + `
'--add Microsoft.VisualStudio.ClientDiagnostics.Resources ' + `
'--add Microsoft.VisualStudio.ProjectSystem.Full ' + `
'--add Microsoft.VisualStudio.LiveShareApi ' + `
'--add Microsoft.VisualStudio.ProjectSystem.Query ' + `
'--add Microsoft.VisualStudio.ProjectSystem ' + `
'--add Microsoft.VisualStudio.Community.x86 ' + `
'--add Microsoft.VisualStudio.Community.x64 ' + `
'--add Microsoft.VisualStudio.Community ' + `
'--add Microsoft.IntelliTrace.CollectorCab ' + `
'--add Microsoft.VisualStudio.Community.Resources ' + `
'--add Microsoft.VisualStudio.WebSiteProject.DTE ' + `
'--add Microsoft.VisualStudio.Platform.CallHierarchy ' + `
'--add Microsoft.VisualStudio.Community.Msi ' + `
'--add Microsoft.VisualStudio.MinShell.Interop.Msi ' + `
'--add Microsoft.VisualStudio.Editors ' + `
'--add Microsoft.NetCore.Component.SDK ' + `
'--add Microsoft.Net.Core.SDK.MSBuildExtensions ' + `
'--add Microsoft.NetStandard.TargetingPack.2.1.2.1.0 ' + `
'--add Microsoft.NetCore.Toolset.3.1.301 ' + `
'--add Microsoft.NetCore.SharedHost.3.1.5 ' + `
'--add Microsoft.NetCore.HostFXR.3.1.5.x86 ' + `
'--add Microsoft.NetCore.HostFXR.3.1.5.x64 ' + `
'--add Microsoft.NetCore.Component.Runtime.3.1 ' + `
'--add Microsoft.WindowsDesktop.TargetingPack.3.1.3.1.0 ' + `
'--add Microsoft.WindowsDesktop.SharedFramework.3.1.3.1.5.x86 ' + `
'--add Microsoft.WindowsDesktop.SharedFramework.3.1.3.1.5.x64 ' + `
'--add Microsoft.PackageGroup.ClientDiagnostics ' + `
'--add Microsoft.NetCore.Templates.3.1.3.1.301-servicing-015174 ' + `
'--add Microsoft.NetCore.TargetingPack.3.1.3.1.0 ' + `
'--add Microsoft.NetCore.SharedFramework.3.1.3.1.5.x86 ' + `
'--add Microsoft.NetCore.SharedFramework.3.1.3.1.5.x64 ' + `
'--add Microsoft.NetCore.AppHostPack.3.1.3.1.5.x86 ' + `
'--add Microsoft.NetCore.AppHostPack.3.1.3.1.5.x64 ' + `
'--add Microsoft.VisualStudio.PackageGroup.CommunityCore ' + `
'--add Microsoft.NetCore.AppHostPack.3.1.3.1.5.arm64 ' + `
'--add Microsoft.NetCore.AppHostPack.3.1.3.1.5.arm ' + `
'--add Microsoft.AspNetCore.TargetingPack.3.1.3.1.3 ' + `
'--add Microsoft.AspNetCore.SharedFramework.3.1.3.1.5-servicing.20271.5.x86 ' + `
'--add Microsoft.AspNetCore.SharedFramework.3.1.3.1.5-servicing.20271.5.x64 ' + `
'--add Microsoft.NetCore.SdkPlaceholder.3.1.301-servicing-015174 ' + `
'--add Microsoft.VisualStudio.PackageGroup.CoreEditor ' + `
'--add Microsoft.VisualStudio.Net.Eula.Resources ' + `
'--add Microsoft.VisualCpp.Tools.Common.UtilsPrereq ' + `
'--add Microsoft.MSHtml ' + `
'--add Microsoft.VisualCpp.Tools.Common.Utils.Resources ' + `
'--add Microsoft.VisualStudio.Community.Msi.Resources ' + `
'--add Microsoft.VisualStudio.Devenv.Msi ' + `
'--add Microsoft.VisualCpp.Tools.Common.Utils ' + `
'--add Microsoft.VisualStudio.Graphics.Msi ' + `
'--add Microsoft.VisualStudio.Component.CoreEditor ' + `
'--add Microsoft.VisualStudio.VC.DevCmd ' + `
'--add Microsoft.VisualStudio.Workload.CoreEditor ' + `
'--add Microsoft.VisualStudio.VC.DevCmd.Resources ' + `
'--add Microsoft.VisualStudio.VirtualTree ' + `
'--add Microsoft.VisualStudio.PerformanceProvider ' + `
'--add Microsoft.VisualStudio.GraphProvider ' + `
'--add Microsoft.DiaSymReader ' + `
'--add Microsoft.VisualStudio.PackageGroup.VsDevCmd ' + `
'--add Microsoft.VisualStudio.GraphModel ' + `
'--add Microsoft.VisualStudio.VsDevCmd.Core.WinSdk ' + `
'--add Microsoft.VisualStudio.VsDevCmd.Core.DotNet ' + `
'--add Microsoft.Build.Dependencies ' + `
'--add Microsoft.VisualStudio.NuGet.Core ' + `
'--add Microsoft.VisualStudio.TextMateGrammars ' + `
'--add Microsoft.VisualStudio.PackageGroup.Progression ' + `
'--add Microsoft.Build.FileTracker.Msi ' + `
'--add Microsoft.Build ' + `
'--add Microsoft.VisualStudio.Platform.CrossRepositorySearch ' + `
'--add Microsoft.VisualStudio.TeamExplorer ' + `
'--add Microsoft.VisualStudio.PackageGroup.NuGet ' + `
'--add Microsoft.VisualStudio.PackageGroup.TeamExplorer.Common ' + `
'--add Microsoft.ServiceHub ' + `
'--add Microsoft.VisualStudio.ProjectServices ' + `
'--add Microsoft.VisualStudio.OpenFolder.VSIX ' + `
'--add Microsoft.VisualStudio.FileHandler.Msi ' + `
'--add Microsoft.VisualStudio.FileHandler.Msi ' + `
'--add Microsoft.VisualStudio.PackageGroup.MinShell ' + `
'--add Microsoft.VisualStudio.MinShell.Interop ' + `
'--add Microsoft.VisualStudio.Log ' + `
'--add Microsoft.VisualStudio.Graphics.Analyzer ' + `
'--add Microsoft.VisualStudio.Finalizer ' + `
'--add Microsoft.VisualStudio.CoreEditor ' + `
'--add Microsoft.VisualStudio.Devenv ' + `
'--add Microsoft.VisualStudio.Devenv.Resources ' + `
'--add Microsoft.VisualStudio.Platform.NavigateTo ' + `
'--add Microsoft.VisualStudio.Log.Targeted ' + `
'--add Microsoft.VisualStudio.Log.Resources ' + `
'--add Microsoft.VisualStudio.Connected ' + `
'--add Microsoft.VisualStudio.AzureSDK ' + `
'--add Microsoft.VisualStudio.PerfLib ' + `
'--add Microsoft.VisualStudio.Connected.Resources ' + `
'--add Microsoft.VisualStudio.MinShell ' + `
'--add Microsoft.VisualStudio.Setup.WMIProvider ' + `
'--add Microsoft.VisualStudio.MinShell.Platform ' + `
'--add Microsoft.VisualStudio.MinShell.Platform.Resources ' + `
'--add Microsoft.VisualStudio.Setup.Configuration ' + `
'--add Microsoft.VisualStudio.Graphics.Analyzer.Resources ' + `
'--add Microsoft.VisualStudio.LanguageServer ' + `
'--add Microsoft.VisualStudio.Platform.Terminal ' + `
'--add Microsoft.VisualStudio.MefHosting.Resources ' + `
'--add Microsoft.VisualStudio.Initializer ' + `
'--add Microsoft.VisualStudio.ExtensionManager ' + `
'--add Microsoft.VisualStudio.Platform.Editor ' + `
'--add Microsoft.VisualStudio.MinShell.x86 ' + `
'--add Microsoft.VisualStudio.VsWebProtocolSelector.Msi ' + `
'--add Microsoft.VisualStudio.NativeImageSupport ' + `
'--add Microsoft.VisualStudio.MinShell.Msi ' + `
'--add Microsoft.VisualStudio.MinShell.Msi.Resources ' + `
'--add Microsoft.VisualStudio.MefHosting ' + `
'--add Microsoft.VisualStudio.Devenv.Config ' + `
'--add Microsoft.VisualStudio.MinShell.Resources ' + `
'--add Microsoft.Net.PackageGroup.4.7.2.Redist ' + `
'--add Microsoft.VisualStudio.Branding.Community ' + `
'--add Microsoft.VisualStudio.Product.BuildTools ' + `
'--add Microsoft.VisualStudio.Workload.VCTools ' + `
'--add Microsoft.VisualStudio.Component.TestTools.BuildTools ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.BuildTools ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.V1.CLI ' + `
'--add Microsoft.VisualStudio.PackageGroup.TestTools.TestPlatform.V2.CLI ' + `
'--add Microsoft.VisualStudio.TestTools.TestPlatform.V2.CLI ' + `
'--add Microsoft.VisualStudio.Component.VC.CMake.Project ' + `
'--add Microsoft.VisualStudio.VC.CMake ' + `
'--add Microsoft.VisualStudio.Component.Windows10SDK.17763 ' + `
'--add MLGen ' + `
'--add Win10SDK_10.0.17763 ' + `
'--add Microsoft.VisualStudio.Component.VC.Redist.14.Latest ' + `
'--add Microsoft.VisualCpp.Redist.14.Latest ' + `
'--add Microsoft.VisualCpp.Redist.14.Latest ' + `
'--add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.Extensions ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.Extensions.X86 ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.ConcurrencyCheck.X86 ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.ConcurrencyCheck.X86.Resources ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.Extensions.X64 ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.ConcurrencyCheck.X64 ' + `
'--add Microsoft.VisualCpp.CodeAnalysis.ConcurrencyCheck.X64.Resources ' + `
'--add Microsoft.VisualStudio.Component.Static.Analysis.Tools ' + `
'--add Microsoft.VisualStudio.StaticAnalysis ' + `
'--add Microsoft.VisualStudio.StaticAnalysis.Resources ' + `
'--add Microsoft.VisualCpp.Tools.HostX64.TargetX86 ' + `
'--add Microsoft.VisualCpp.VCTip.HostX64.TargetX86 ' + `
'--add Microsoft.VisualCpp.Tools.HostX64.TargetX86.Resources ' + `
'--add Microsoft.VisualCpp.Tools.HostX64.TargetX64 ' + `
'--add Microsoft.VisualCpp.VCTip.HostX64.TargetX64 ' + `
'--add Microsoft.VisualCpp.Tools.HostX64.TargetX64.Resources ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX86.TargetX64 ' + `
'--add Microsoft.VisualCpp.Premium.Tools.Hostx86.Targetx64.Resources ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX86.TargetX86 ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX86.TargetX86.Resources ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX64.TargetX86 ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX64.TargetX86.Resources ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX64.TargetX64 ' + `
'--add Microsoft.VisualCpp.Premium.Tools.HostX64.TargetX64.Resources ' + `
'--add Microsoft.VisualCpp.PGO.X86 ' + `
'--add Microsoft.VisualCpp.PGO.X64 ' + `
'--add Microsoft.VisualCpp.PGO.Headers ' + `
'--add Microsoft.VisualCpp.CRT.x86.Store ' + `
'--add Microsoft.VisualCpp.CRT.x86.OneCore.Desktop ' + `
'--add Microsoft.VisualCpp.CRT.x64.Store ' + `
'--add Microsoft.VisualCpp.CRT.x64.OneCore.Desktop ' + `
'--add Microsoft.VisualCpp.CRT.Redist.x86.OneCore.Desktop ' + `
'--add Microsoft.VisualCpp.CRT.Redist.x64.OneCore.Desktop ' + `
'--add Microsoft.VisualCpp.CRT.ClickOnce.Msi ' + `
'--add Microsoft.VisualStudio.PackageGroup.VC.Tools.x86 ' + `
'--add Microsoft.VisualCpp.Tools.HostX86.TargetX64 ' + `
'--add Microsoft.VisualCpp.VCTip.hostX86.targetX64 ' + `
'--add Microsoft.VisualCpp.Tools.Hostx86.Targetx64.Resources ' + `
'--add Microsoft.VisualCpp.Tools.HostX86.TargetX86 ' + `
'--add Microsoft.VisualCpp.VCTip.hostX86.targetX86 ' + `
'--add Microsoft.VisualCpp.Tools.HostX86.TargetX86.Resources ' + `
'--add Microsoft.VisualCpp.Tools.Core.Resources ' + `
'--add Microsoft.VisualCpp.Tools.Core.x86 ' + `
'--add Microsoft.VisualCpp.Tools.Common.Utils ' + `
'--add Microsoft.VisualCpp.Tools.Common.Utils.Resources ' + `
'--add Microsoft.VisualCpp.DIA.SDK ' + `
'--add Microsoft.VisualCpp.CRT.x86.Desktop ' + `
'--add Microsoft.VisualCpp.CRT.x64.Desktop ' + `
'--add Microsoft.VisualCpp.CRT.Source ' + `
'--add Microsoft.VisualCpp.CRT.Redist.X86 ' + `
'--add Microsoft.VisualCpp.CRT.Redist.X64 ' + `
'--add Microsoft.VisualCpp.CRT.Redist.Resources ' + `
'--add Microsoft.VisualCpp.RuntimeDebug.14 ' + `
'--add Microsoft.VisualCpp.RuntimeDebug.14 ' + `
'--add Microsoft.VisualCpp.Redist.14 ' + `
'--add Microsoft.VisualCpp.Redist.14 ' + `
'--add Microsoft.VisualCpp.CRT.Headers ' + `
'--add Microsoft.VisualStudio.Component.VC.CoreBuildTools ' + `
'--add Microsoft.VisualStudio.Component.Windows10SDK ' + `
'--add Microsoft.Windows.UniversalCRT.Redistributable.Msi ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.X86 ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.X64 ' + `
'--add Microsoft.VS.VC.MSBuild.X64.Resources ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.Base ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.Base.Resources ' + `
'--add Microsoft.VisualStudio.VC.MSBuild.ARM ' + `
'--add Microsoft.VisualStudio.Workload.MSBuildTools ' + `
'--add Microsoft.VisualStudio.Component.CoreBuildTools ' + `
'--add Microsoft.VisualStudio.Setup.WMIProvider ' + `
'--add Microsoft.VisualStudio.Setup.Configuration ' + `
'--add Microsoft.VisualStudio.PackageGroup.VsDevCmd ' + `
'--add Microsoft.VisualStudio.VsDevCmd.Ext.NetFxSdk ' + `
'--add Microsoft.VisualStudio.VsDevCmd.Core.WinSdk ' + `
'--add Microsoft.VisualStudio.VsDevCmd.Core.DotNet ' + `
'--add Microsoft.VisualStudio.VC.DevCmd ' + `
'--add Microsoft.VisualStudio.VC.DevCmd.Resources ' + `
'--add Microsoft.VisualStudio.BuildTools.Resources ' + `
'--add Microsoft.VisualStudio.Net.Eula.Resources ' + `
'--add Microsoft.Build.Dependencies ' + `
'--add Microsoft.Build.FileTracker.Msi ' + `
'--add Microsoft.Component.MSBuild ' + `
'--add Microsoft.PythonTools.BuildCore.Vsix ' + `
'--add Microsoft.NuGet.Build.Tasks ' + `
'--add Microsoft.VisualStudio.Component.Roslyn.Compiler ' + `
'--add Microsoft.CodeAnalysis.Compilers.Resources ' + `
'--add Microsoft.CodeAnalysis.Compilers ' + `
'--add Microsoft.Net.PackageGroup.4.6.1.Redist ' + `
'--add Microsoft.VisualStudio.NativeImageSupport ' + `
'--add Microsoft.Build '

$Sku = 'Community'
$VSBootstrapperURL = 'https://aka.ms/vs/16/release/vs_community.exe'

$ErrorActionPreference = 'Stop'

Write-Host "Step 1. Setting parameters... Done!" -ForegroundColor Cyan

# TODO: Check VS and Workloads and then make installation only for missed workloads
#Install-Module VSSetup -Scope CurrentUser
#if((Get-VSSetupInstance | Select-VSSetupInstance -Product *) | select -ExpandProperty DisplayName | Where-Object {($_ -like '*2019*')}) { 
#    Write-Host "Visual Studio 2019 is already installed! But it have to have "
#} else { 
#
#}

Write-Host "Step 2. Starting the Installation process..." -ForegroundColor Cyan
# Install VS
$exitCode = InstallVS -WorkLoads $WorkLoads -Sku $Sku -VSBootstrapperURL $VSBootstrapperURL

Write-Host "Step 2. Starting the Installation process... Done!" -ForegroundColor Cyan

Write-Host "Step 3. Rebooting possibly affected services..." -ForegroundColor Cyan
if (get-Service SQLWriterw -ErrorAction Ignore) {
  Stop-Service SQLWriter
  Set-Service SQLWriter -StartupType Manual
}
if (get-Service IpOverUsbSvc -ErrorAction Ignore) {
  Stop-Service IpOverUsbSvc
  Set-Service IpOverUsbSvc -StartupType Manual
}
Write-Host "Step 3. Rebooting possibly affected services... Done!" -ForegroundColor Cyan

Write-Host "Step 4. Adding Visual Studio 2019 current MSBuild to PATH..." -ForegroundColor Cyan
setx PATH "$env:path;${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin" -m
Write-Host "Step 4. Adding Visual Studio 2019 current MSBuild to PATH... Done!" -ForegroundColor Cyan