# GitHub Actions

In order to be able to use these Workflows, the repository needs to be set up:

- Permissions
  - Actions: Allow all actions and resuable workflows
  - Workflows: Read and write permissions
- Repository secrets:
  - `MACOS_CODESIGN_CERTIFICATE`  
    Exported "Developer ID Application" Certificate as a .p12 file, Base64 Encoded
    - Export "Developer ID Application" Certificate as a .p12 file *(and set a password for the .p12 file)*
    - Use the following command to convert the .p12 certificate to Base64 and copy it to the clipboard:  
      `base64 -i BUILD_CERTIFICATE.p12 | pbcopy`
  - `MACOS_CODESIGN_CERTIFICATE_PASSWORD`  
      Password for the exported "Developer ID Application" certificate
  - `MACOS_CODESIGN_KEYCHAIN_PASSWORD`  
    Anything... only used in the macOS Runner to set up a Keychain with the Certificate
  - `MACOS_CODESIGN_DEVELOPMENT_TEAM`  
    See in your Apple Developer Account: Membership Details
  - `MACOS_CODESIGN_IDENTITY`  
    The display name of the Certificate, such as in Keychain.app, e.g.: `Developer ID Application: Your Name`
