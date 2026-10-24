"""Read and verify the fixed low-autonomy execution profile used by workspace builds."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_PROFILE = ROOT / "docs/designRules/architecture/agent-execution-capability-profile.json"
REQUIRED_TOP_LEVEL = {
    "schemaVersion", "kind", "displayName", "canonicalId", "observedDate",
    "runtime", "capability", "permittedWorkloads", "restrictedWorkloads",
    "executionContract",
}
REQUIRED_CONTRACT = {
    "executionClass", "readBeforeEdit", "testAfterEdit", "evidenceRequired",
    "externalFactCheckRequired", "autonomousEnvironmentChange", "completionRequiresArtifact",
    "requiresFrozenJobSpecForGeneration", "requiresExplicitPathsProfilesAndModes",
    "forbidsInventedDesignValues", "forbidsFallbackGeneratorOrParser",
    "forbidsImplicitProfileOrSdkSwitch", "failureAction",
}


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_profile(path=DEFAULT_PROFILE):
    path = Path(path).resolve()
    try:
        profile = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError("AGENT_CAPABILITY_PROFILE_UNREADABLE: " + str(path)) from error
    missing = REQUIRED_TOP_LEVEL - profile.keys()
    if missing:
        raise ValueError("AGENT_CAPABILITY_PROFILE_FIELDS_MISSING: " + ",".join(sorted(missing)))
    if profile["schemaVersion"] != 1 or profile["kind"] != "agent-execution-capability-profile":
        raise ValueError("AGENT_CAPABILITY_PROFILE_SCHEMA_INVALID")
    if not isinstance(profile["canonicalId"], str) or not profile["canonicalId"].strip():
        raise ValueError("AGENT_CAPABILITY_PROFILE_ID_INVALID")
    contract = profile["executionContract"]
    if not isinstance(contract, dict) or REQUIRED_CONTRACT - contract.keys():
        raise ValueError("AGENT_CAPABILITY_PROFILE_CONTRACT_INVALID")
    required_true = (
        "readBeforeEdit", "testAfterEdit", "evidenceRequired", "externalFactCheckRequired",
        "completionRequiresArtifact", "requiresFrozenJobSpecForGeneration",
        "requiresExplicitPathsProfilesAndModes", "forbidsInventedDesignValues",
        "forbidsFallbackGeneratorOrParser", "forbidsImplicitProfileOrSdkSwitch",
    )
    if any(contract[key] is not True for key in required_true):
        raise ValueError("AGENT_CAPABILITY_PROFILE_GUARD_WEAKENED")
    if contract["autonomousEnvironmentChange"] is not False or contract["failureAction"] != "record-evidence-and-stop":
        raise ValueError("AGENT_CAPABILITY_PROFILE_FAILURE_POLICY_INVALID")
    return profile, path


def identity(path=DEFAULT_PROFILE):
    profile, resolved = read_profile(path)
    return {
        "canonicalId": profile["canonicalId"],
        "displayName": profile["displayName"],
        "executionClass": profile["executionContract"]["executionClass"],
        "profileSha256": sha256(resolved),
        "profilePath": str(resolved),
    }


def binding(path=DEFAULT_PROFILE):
    """Portable receipt data: identity plus bytes, never a machine-specific path."""
    result = identity(path)
    result.pop("profilePath")
    return result


def main():
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("show", "verify"))
    parser.add_argument("--profile", type=Path, default=DEFAULT_PROFILE)
    args = parser.parse_args()
    result = identity(args.profile)
    if args.action == "verify":
        result = {"status": "PASS", **result}
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))


if __name__ == "__main__":
    main()
